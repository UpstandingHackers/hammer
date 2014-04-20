#include <assert.h>
#include "contextfree.h"
#include "lr.h"



/* LALR-via-SLR grammar transformation */

static inline size_t seqsize(void *p_)
{
  size_t n=0;
  for(void **p=p_; *p; p++) n++;
  return n+1;
}

static HLRAction *
lrtable_lookup(const HLRTable *table, size_t state, const HCFChoice *symbol)
{
  switch(symbol->type) {
  case HCF_END:
    return table->tmap[state]->end_branch;
  case HCF_CHAR:
    return h_stringmap_get(table->tmap[state], &symbol->chr, 1, false);
  default:
    // nonterminal case
    return h_hashtable_get(table->ntmap[state], symbol);
  }
}

static size_t follow_transition(const HLRTable *table, size_t x, HCFChoice *A)
{
  HLRAction *action = lrtable_lookup(table, x, A);
  assert(action != NULL);
  assert(action->type == HLR_SHIFT);
  return action->nextstate;
}

static inline HLRTransition *transition(HArena *arena,
                                        size_t x, const HCFChoice *A, size_t y)
{
  HLRTransition *t = h_arena_malloc(arena, sizeof(HLRTransition));
  t->from = x;
  t->symbol = A;
  t->to = y;
  return t;
}

// no-op on terminal symbols
static void transform_productions(const HLRTable *table, HLREnhGrammar *eg,
                                  size_t x, HCFChoice *xAy)
{
  if (xAy->type != HCF_CHOICE) {
    return;
  }
  // XXX CHARSET?

  HArena *arena = eg->arena;

  HCFSequence **seq = h_arena_malloc(arena, seqsize(xAy->seq)
                                            * sizeof(HCFSequence *));
  HCFSequence **p, **q;
  for(p=xAy->seq, q=seq; *p; p++, q++) {
    // trace rhs starting in state x and following the transitions
    // xAy -> ... iBj ...

    size_t i = x;
    HCFChoice **B = (*p)->items;
    HCFChoice **items = h_arena_malloc(arena, seqsize(B) * sizeof(HCFChoice *));
    HCFChoice **iBj = items;
    for(; *B; B++, iBj++) {
      size_t j = follow_transition(table, i, *B);
      HLRTransition *i_B_j = transition(arena, i, *B, j);
      *iBj = h_hashtable_get(eg->tmap, i_B_j);
      assert(*iBj != NULL);
      i = j;
    }
    *iBj = NULL;

    *q = h_arena_malloc(arena, sizeof(HCFSequence));
    (*q)->items = items;
  }
  *q = NULL;
  xAy->seq = seq;
}

static HCFChoice *new_enhanced_symbol(HLREnhGrammar *eg, const HCFChoice *sym)
{
  HArena *arena = eg->arena;
  HCFChoice *esym = h_arena_malloc(arena, sizeof(HCFChoice));
  *esym = *sym;

  HHashSet *cs = h_hashtable_get(eg->corr, sym);
  if (!cs) {
    cs = h_hashset_new(arena, h_eq_symbol, h_hash_symbol);
    h_hashtable_put(eg->corr, sym, cs);
  }
  h_hashset_put(cs, esym);

  return esym;
}

static HLREnhGrammar *enhance_grammar(const HCFGrammar *g, const HLRDFA *dfa,
                                      const HLRTable *table)
{
  HAllocator *mm__ = g->mm__;
  HArena *arena = g->arena;

  HLREnhGrammar *eg = h_arena_malloc(arena, sizeof(HLREnhGrammar));
  eg->tmap = h_hashtable_new(arena, h_eq_transition, h_hash_transition);
  eg->smap = h_hashtable_new(arena, h_eq_ptr, h_hash_ptr);
  eg->corr = h_hashtable_new(arena, h_eq_symbol, h_hash_symbol);
  // XXX must use h_eq/hash_ptr for symbols! so enhanced CHARs are different
  eg->arena = arena;

  // establish mapping between transitions and symbols
  for(HSlistNode *x=dfa->transitions->head; x; x=x->next) {
    HLRTransition *t = x->elem;

    assert(!h_hashtable_present(eg->tmap, t));

    HCFChoice *sym = new_enhanced_symbol(eg, t->symbol);
    h_hashtable_put(eg->tmap, t, sym);
    h_hashtable_put(eg->smap, sym, t);
  }

  // transform the productions
  H_FOREACH(eg->tmap, HLRTransition *t, HCFChoice *sym)
    transform_productions(table, eg, t->from, sym);
  H_END_FOREACH

  // add the start symbol
  HCFChoice *start = new_enhanced_symbol(eg, g->start);
  transform_productions(table, eg, 0, start);

  eg->grammar = h_cfgrammar_(mm__, start);
  return eg;
}



/* LALR table generation */

static inline bool has_conflicts(HLRTable *table)
{
  return !h_slist_empty(table->inadeq);
}

// for each lookahead symbol (fs), put action into tmap
// returns 0 on success, -1 on conflict
// ignores forall entries
static int terminals_put(HStringMap *tmap, const HStringMap *fs, HLRAction *action)
{
  int ret = 0;

  if (fs->epsilon_branch) {
    HLRAction *prev = tmap->epsilon_branch;
    if (prev && prev != action) {
      // conflict
      tmap->epsilon_branch = h_lr_conflict(tmap->arena, prev, action);
      ret = -1;
    } else {
      tmap->epsilon_branch = action;
    }
  }

  if (fs->end_branch) {
    HLRAction *prev = tmap->end_branch;
    if (prev && prev != action) {
      // conflict
      tmap->end_branch = h_lr_conflict(tmap->arena, prev, action);
      ret = -1;
    } else {
      tmap->end_branch = action;
    }
  }

  H_FOREACH(fs->char_branches, void *key, HStringMap *fs_)
    HStringMap *tmap_ = h_hashtable_get(tmap->char_branches, key);

    if (!tmap_) {
      tmap_ = h_stringmap_new(tmap->arena);
      h_hashtable_put(tmap->char_branches, key, tmap_);
    }

    if (terminals_put(tmap_, fs_, action) < 0) {
      ret = -1;
    }
  H_END_FOREACH

  return ret;
}

// check whether a sequence of enhanced-grammar symbols (p) matches the given
// (original-grammar) production rhs and terminates in the given end state.
static bool match_production(HLREnhGrammar *eg, HCFChoice **p,
                             HCFChoice **rhs, size_t endstate)
{
  size_t state = endstate;  // initialized to end in case of empty rhs
  for(; *p && *rhs; p++, rhs++) {
    HLRTransition *t = h_hashtable_get(eg->smap, *p);
    assert(t != NULL);
    if (!h_eq_symbol(t->symbol, *rhs)) {
      return false;
    }
    state = t->to;
  }
  return (*p == *rhs    // both NULL
          && state == endstate);
}

// desugar parser with a fresh start symbol
// this guarantees that the start symbol will not occur in any productions
HCFChoice *h_desugar_augmented(HAllocator *mm__, HParser *parser)
{
  HCFChoice *augmented = h_new(HCFChoice, 1);

  HCFStack *stk__ = h_cfstack_new(mm__);
  stk__->prealloc = augmented;
  HCFS_BEGIN_CHOICE() {
    HCFS_BEGIN_SEQ() {
      HCFS_DESUGAR(parser);
    } HCFS_END_SEQ();
    HCFS_THIS_CHOICE->reshape = h_act_first;
  } HCFS_END_CHOICE();
  h_cfstack_free(mm__, stk__);

  return augmented;
}

int h_lalr_compile(HAllocator* mm__, HParser* parser, const void* params)
{
  // generate (augmented) CFG from parser
  // construct LR(0) DFA
  // build LR(0) table
  // if necessary, resolve conflicts "by conversion to SLR"

  if (!parser->vtable->isValidCF(parser->env)) {
    return -1;
  }
  HCFGrammar *g = h_cfgrammar_(mm__, h_desugar_augmented(mm__, parser));
  if(g == NULL)     // backend not suitable (language not context-free)
    return -1;

  HLRDFA *dfa = h_lr0_dfa(g);
  if (dfa == NULL) {     // this should normally not happen
    h_cfgrammar_free(g);
    return -1;
  }

  HLRTable *table = h_lr0_table(g, dfa);
  if (table == NULL) {   // this should normally not happen
    h_cfgrammar_free(g);
    return -1;
  }

  if(has_conflicts(table)) {
    HArena *arena = table->arena;

    HLREnhGrammar *eg = enhance_grammar(g, dfa, table);
    if(eg == NULL) {    // this should normally not happen
      h_cfgrammar_free(g);
      h_lrtable_free(table);
      return -1;
    }

    // go through the inadequate states; replace inadeq with a new list
    HSlist *inadeq = table->inadeq;
    table->inadeq = h_slist_new(arena);
    
    for(HSlistNode *x=inadeq->head; x; x=x->next) {
      size_t state = (uintptr_t)x->elem;
      bool inadeq = false;
      
      // clear old forall entry, it's being replaced by more fine-grained ones
      table->forall[state] = NULL;

      // go through each reducible item of state
      H_FOREACH_KEY(dfa->states[state], HLRItem *item)
        if(item->mark < item->len)
          continue;

        // action to place in the table cells indicated by lookahead
        HLRAction *action = h_reduce_action(arena, item);

        // find all LR(0)-enhanced productions matching item
        HHashSet *lhss = h_hashtable_get(eg->corr, item->lhs);
        assert(lhss != NULL);
        H_FOREACH_KEY(lhss, HCFChoice *lhs)
          assert(lhs->type == HCF_CHOICE);  // XXX could be CHARSET?

	  for(HCFSequence **p=lhs->seq; *p; p++) {
            HCFChoice **rhs = (*p)->items;
            if(!match_production(eg, rhs, item->rhs, state)) {
              continue;
	    }

            // the left-hand symbol's follow set is this production's
            // contribution to the lookahead
            const HStringMap *fs = h_follow(1, eg->grammar, lhs);
            assert(fs != NULL);
            assert(fs->epsilon_branch == NULL);
            assert(!h_stringmap_empty(fs));

            // for each lookahead symbol, put action into table cell
            if(terminals_put(table->tmap[state], fs, action) < 0)
              inadeq = true;
	  } H_END_FOREACH // enhanced production
      H_END_FOREACH  // reducible item

      if(inadeq) {
        h_slist_push(table->inadeq, (void *)(uintptr_t)state);
      }
    }
  }

  h_cfgrammar_free(g);
  parser->backend_data = table;
  return has_conflicts(table)? -1 : 0;
}

void h_lalr_free(HParser *parser)
{
  HLRTable *table = parser->backend_data;
  h_lrtable_free(table);
  parser->backend_data = NULL;
  parser->backend = PB_PACKRAT;
}



HParserBackendVTable h__lalr_backend_vtable = {
  .compile = h_lalr_compile,
  .parse = h_lr_parse,
  .free = h_lalr_free
};




// dummy!
int test_lalr(void)
{
  HAllocator *mm__ = &system_allocator;

  /* 
     E -> E '-' T
        | T
     T -> '(' E ')'
        | 'n'               -- also try [0-9] for the charset paths
  */

  HParser *n = h_ch('n');
  HParser *E = h_indirect();
  HParser *T = h_choice(h_sequence(h_ch('('), E, h_ch(')'), NULL), n, NULL);
  HParser *E_ = h_choice(h_sequence(E, h_ch('-'), T, NULL), T, NULL);
  h_bind_indirect(E, E_);
  HParser *p = E;

  printf("\n==== G R A M M A R ====\n");
  HCFGrammar *g = h_cfgrammar_(mm__, h_desugar_augmented(mm__, p));
  if (g == NULL) {
    fprintf(stderr, "h_cfgrammar failed\n");
    return 1;
  }
  h_pprint_grammar(stdout, g, 0);

  printf("\n==== D F A ====\n");
  HLRDFA *dfa = h_lr0_dfa(g);
  if (dfa) {
    h_pprint_lrdfa(stdout, g, dfa, 0);
  } else {
    fprintf(stderr, "h_lalr_dfa failed\n");
  }

  printf("\n==== L R ( 0 )  T A B L E ====\n");
  HLRTable *table0 = h_lr0_table(g, dfa);
  if (table0) {
    h_pprint_lrtable(stdout, g, table0, 0);
  } else {
    fprintf(stderr, "h_lr0_table failed\n");
  }
  h_lrtable_free(table0);

  printf("\n==== L A L R  T A B L E ====\n");
  if (h_compile(p, PB_LALR, NULL)) {
    fprintf(stderr, "does not compile\n");
    return 2;
  }
  h_pprint_lrtable(stdout, g, (HLRTable *)p->backend_data, 0);

  printf("\n==== P A R S E  R E S U L T ====\n");
  HParseResult *res = h_parse(p, (uint8_t *)"n-(n-((n)))-n", 13);
  if (res) {
    h_pprint(stdout, res->ast, 0, 2);
  } else {
    printf("no parse\n");
  }
  return 0;
}
