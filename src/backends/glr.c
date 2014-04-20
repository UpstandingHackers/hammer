#include <assert.h>
#include "lr.h"

static bool glr_step(HParseResult **result, HSlist *engines,
                     HLREngine *engine, const HLRAction *action);


/* GLR compilation (LALR w/o failing on conflict) */

int h_glr_compile(HAllocator* mm__, HParser* parser, const void* params)
{
  if (!parser->vtable->isValidCF(parser->env)) {
    return -1;
  }
  int result = h_lalr_compile(mm__, parser, params);

  if(result == -1 && parser->backend_data) {
    // table is there, just has conflicts? nevermind, that's okay.
    result = 0;
  }

  return result;
}

void h_glr_free(HParser *parser)
{
  h_lalr_free(parser);
}


/* Merging engines (when they converge on the same state) */

static HLREngine *lrengine_merge(HLREngine *old, HLREngine *new)
{
  HArena *arena = old->arena;

  HLREngine *ret = h_arena_malloc(arena, sizeof(HLREngine));

  assert(old->state == new->state);
  assert(old->input.input == new->input.input);

  *ret = *old;
  ret->stack = h_slist_new(arena);
  ret->merged[0] = old;
  ret->merged[1] = new;

  return ret;
}

static HSlist *demerge_stack(HSlistNode *bottom, HSlist *stack)
{
  HArena *arena = stack->arena;

  HSlist *ret = h_slist_new(arena);

  // copy the stack from the top
  HSlistNode **y = &ret->head;
  for(HSlistNode *x=stack->head; x; x=x->next) {
    HSlistNode *node = h_arena_malloc(arena, sizeof(HSlistNode));
    node->elem = x->elem;
    node->next = NULL;
    *y = node;
    y = &node->next;
  }
  *y = bottom;  // attach the ancestor stack

  return ret;
}

static inline HLREngine *respawn(HLREngine *eng, HSlist *stack)
{
  // NB: this can be a destructive update because an engine is not used for
  // anything after it is merged.
  eng->stack = demerge_stack(eng->stack->head, stack);
  return eng;
}

static HLREngine *
demerge(HParseResult **result, HSlist *engines,
        HLREngine *engine, const HLRAction *action, size_t depth)
{
  // no-op on engines that are not merged
  if(!engine->merged[0])
    return engine;

  HSlistNode *p = engine->stack->head;
  for(size_t i=0; i<depth; i++) {
    // if stack hits bottom, respawn ancestors
    if(p == NULL) {
      HLREngine *a = respawn(engine->merged[0], engine->stack);
      HLREngine *b = respawn(engine->merged[1], engine->stack);

      // continue demerge until final depth reached
      a = demerge(result, engines, a, action, depth-i);
      b = demerge(result, engines, b, action, depth-i);
      
      // step and stow one ancestor...
      glr_step(result, engines, a, action);

      // ...and return the other
      return b;
    }
    p = p->next;
  }

  return engine;    // there is enough stack before the merge point
}


/* Forking engines (on conflicts */

HLREngine *fork_engine(const HLREngine *engine)
{
  HLREngine *eng2 = h_arena_malloc(engine->tarena, sizeof(HLREngine));
  eng2->table = engine->table;
  eng2->state = engine->state;
  eng2->input = engine->input;

  // shallow-copy the stack
  // this works because h_slist_push and h_slist_drop never modify
  // the underlying structure of HSlistNodes, only the head pointer.
  // in fact, this gives us prefix sharing for free.
  eng2->stack = h_arena_malloc(engine->tarena, sizeof(HSlist));
  *eng2->stack = *engine->stack;

  eng2->arena = engine->arena;
  eng2->tarena = engine->tarena;
  return eng2;
}

static const HLRAction *
handle_conflict(HParseResult **result, HSlist *engines,
                const HLREngine *engine, const HSlist *branches)
{
  // there should be at least two conflicting actions
  assert(branches->head);
  assert(branches->head->next);     // this is just a consistency check

  // fork a new engine for all but the first action
  for(HSlistNode *x=branches->head->next; x; x=x->next) {
    HLRAction *act = x->elem; 
    HLREngine *eng = fork_engine(engine);

    // perform one step and add to engines
    glr_step(result, engines, eng, act);
  } 

  // return first action for use with original engine
  return branches->head->elem;
}


/* GLR driver */

static bool glr_step(HParseResult **result, HSlist *engines,
                     HLREngine *engine, const HLRAction *action)
{
  // handle forks and demerges (~> spawn engines)
  if(action) {
    if(action->type == HLR_CONFLICT) {
      // fork engine on conflicts
      action = handle_conflict(result, engines, engine, action->branches);
    } else if(action->type == HLR_REDUCE) {
      // demerge/respawn as needed
      size_t depth = action->production.length;
      engine = demerge(result, engines, engine, action, depth);
    }
  }

  bool run = h_lrengine_step(engine, action);
  
  if(run) {
    // store engine in the list, merge if necessary
    HSlistNode *x;
    for(x=engines->head; x; x=x->next) {
      HLREngine *eng = x->elem;
      if(eng->state == engine->state) {
        x->elem = lrengine_merge(eng, engine);
        break;
      }
    }
    if(!x)  // no merge happened
      h_slist_push(engines, engine);
  } else if(engine->state == HLR_SUCCESS) {
    // save the result
    *result = h_lrengine_result(engine);
  }

  return run;
}

HParseResult *h_glr_parse(HAllocator* mm__, const HParser* parser, HInputStream* stream)
{
  HLRTable *table = parser->backend_data;
  if(!table)
    return NULL;

  HArena *arena  = h_new_arena(mm__, 0);    // will hold the results
  HArena *tarena = h_new_arena(mm__, 0);    // tmp, deleted after parse

  // allocate engine lists (will hold one engine per state)
  // these are swapped each iteration
  HSlist *engines = h_slist_new(tarena);
  HSlist *engback = h_slist_new(tarena);

  // create initial engine
  h_slist_push(engines, h_lrengine_new(arena, tarena, table, stream));

  HParseResult *result = NULL;
  while(result == NULL && !h_slist_empty(engines)) {
    assert(h_slist_empty(engback));

    // step all engines
    while(!h_slist_empty(engines)) {
      HLREngine *engine = h_slist_pop(engines);
      const HLRAction *action = h_lrengine_action(engine);
      glr_step(&result, engback, engine, action);
    }

    // swap the lists
    HSlist *tmp = engines;
    engines = engback;
    engback = tmp;
  }

  if(!result)
    h_delete_arena(arena);
  h_delete_arena(tarena);
  return result;
}



HParserBackendVTable h__glr_backend_vtable = {
  .compile = h_glr_compile,
  .parse = h_glr_parse,
  .free = h_glr_free
};




// dummy!
int test_glr(void)
{
  HAllocator *mm__ = &system_allocator;

  /* 
     E -> E '+' E
        | 'd'
  */

  HParser *d = h_ch('d');
  HParser *E = h_indirect();
  HParser *E_ = h_choice(h_sequence(E, h_ch('+'), E, NULL), d, NULL);
  h_bind_indirect(E, E_);
  HParser *p = E;

  printf("\n==== G R A M M A R ====\n");
  HCFGrammar *g = h_cfgrammar_(mm__, h_desugar_augmented(mm__, p));
  if(g == NULL) {
    fprintf(stderr, "h_cfgrammar failed\n");
    return 1;
  }
  h_pprint_grammar(stdout, g, 0);

  printf("\n==== D F A ====\n");
  HLRDFA *dfa = h_lr0_dfa(g);
  if(dfa)
    h_pprint_lrdfa(stdout, g, dfa, 0);
  else
    fprintf(stderr, "h_lalr_dfa failed\n");

  printf("\n==== L R ( 0 )  T A B L E ====\n");
  HLRTable *table0 = h_lr0_table(g, dfa);
  if(table0)
    h_pprint_lrtable(stdout, g, table0, 0);
  else
    fprintf(stderr, "h_lr0_table failed\n");
  h_lrtable_free(table0);

  printf("\n==== L A L R  T A B L E ====\n");
  if(h_compile(p, PB_GLR, NULL)) {
    fprintf(stderr, "does not compile\n");
    return 2;
  }
  h_pprint_lrtable(stdout, g, (HLRTable *)p->backend_data, 0);

  printf("\n==== P A R S E  R E S U L T ====\n");
  HParseResult *res = h_parse(p, (uint8_t *)"d+d+d", 5);
  if(res)
    h_pprint(stdout, res->ast, 0, 2);
  else
    printf("no parse\n");

  return 0;
}
