/* Context-free grammar representation and analysis */

#include "cfgrammar.h"
#include "allocator.h"
#include <assert.h>
#include <ctype.h>


// a special map value for use when the map is used to represent a set
static void * const INSET = (void *)(uintptr_t)1;


HCFGrammar *h_cfgrammar_new(HAllocator *mm__)
{
  HCFGrammar *g = h_new(HCFGrammar, 1);
  assert(g != NULL);

  g->mm__   = mm__;
  g->arena  = h_new_arena(mm__, 0);     // default blocksize
  g->nts    = h_hashset_new(g->arena, h_eq_ptr, h_hash_ptr);
  g->start  = NULL;
  g->geneps = NULL;
  g->first  = NULL;
  g->follow = NULL;
  g->kmax   = 0;    // will be increased as needed by ensure_k

  HStringMap *eps = h_stringmap_new(g->arena);
  h_stringmap_put_epsilon(eps, INSET);
  g->singleton_epsilon = eps;

  return g;
}

void h_cfgrammar_free(HCFGrammar *g)
{
  HAllocator *mm__ = g->mm__;
  h_delete_arena(g->arena);
  h_free(g);
}


// helpers
static void collect_nts(HCFGrammar *grammar, HCFChoice *symbol);
static void collect_geneps(HCFGrammar *grammar);


HCFGrammar *h_cfgrammar(HAllocator* mm__, const HParser *parser)
{
  if (!parser->vtable->isValidCF(parser->env)) {
    return NULL;
  }
  // convert parser to CFG form ("desugar").
  HCFChoice *desugared = h_desugar(mm__, NULL, parser);
  if (desugared == NULL) {
    return NULL;  // -> backend not suitable for this parser
  }
  return h_cfgrammar_(mm__, desugared);
}

HCFGrammar *h_cfgrammar_(HAllocator* mm__, HCFChoice *desugared)
{
  HCFGrammar *g = h_cfgrammar_new(mm__);

  // recursively traverse the desugared form and collect all HCFChoices that
  // represent a nonterminal (type HCF_CHOICE or HCF_CHARSET).
  collect_nts(g, desugared);
  if (h_hashset_empty(g->nts)) {
    // desugared is a terminal. wrap it in a singleton HCF_CHOICE.
    HCFChoice *nt = h_new(HCFChoice, 1);
    nt->type = HCF_CHOICE;
    nt->seq = h_new(HCFSequence *, 2);
    nt->seq[0] = h_new(HCFSequence, 1);
    nt->seq[0]->items = h_new(HCFChoice *, 2);
    nt->seq[0]->items[0] = desugared;
    nt->seq[0]->items[1] = NULL;
    nt->seq[1] = NULL;
    nt->pred = NULL;
    nt->action = NULL;
    nt->reshape = h_act_first;
    h_hashset_put(g->nts, nt);
    g->start = nt;
  } else {
    g->start = desugared;
  }

  // determine which nonterminals generate epsilon
  collect_geneps(g);

  return g;
}

/* Add all nonterminals reachable from symbol to grammar. */
static void collect_nts(HCFGrammar *grammar, HCFChoice *symbol)
{
  HCFSequence **s;  // for the rhs (sentential form) of a production
  HCFChoice **x;    // for a symbol in s

  if (h_hashset_present(grammar->nts, symbol)) {
    return;     // already visited, get out
  }

  switch(symbol->type) {
  case HCF_CHAR:
  case HCF_END:
    break;  // it's a terminal symbol, nothing to do

  case HCF_CHARSET:
    break;  // NB charsets are considered terminal, too

  case HCF_CHOICE:
    // exploiting the fact that HHashSet is also a HHashTable to number the
    // nonterminals.
    // NB top-level (start) symbol gets 0.
    h_hashtable_put(grammar->nts, symbol,
                    (void *)(uintptr_t)grammar->nts->used);

    // each element s of symbol->seq (HCFSequence) represents the RHS of
    // a production. call self on all symbols (HCFChoice) in s.
    for(s = symbol->seq; *s != NULL; s++) {
      for(x = (*s)->items; *x != NULL; x++) {
        collect_nts(grammar, *x);
      }
    }
    break;

  default:  // should not be reachable
    assert_message(0, "unknown HCFChoice type");
  }
}

/* Increase g->kmax if needed, allocating enough first/follow slots. */
static void ensure_k(HCFGrammar *g, size_t k)
{
  if (k <= g->kmax) {
    return;
  }
  // NB: we don't actually use first/follow[0] but allocate it anyway
  // so indices of the array correspond neatly to values of k

  // allocate the new arrays
  HHashTable **first  = h_arena_malloc(g->arena, (k+1)*sizeof(HHashTable *));
  HHashTable **follow = h_arena_malloc(g->arena, (k+1)*sizeof(HHashTable *));

  if (g->kmax > 0) {
    // we are resizing, copy the old tables over
    for(size_t i=0; i<=g->kmax; i++) {
      first[i]  = g->first[i];
      follow[i] = g->follow[i];
    }
  } else {
    // we are initializing, allocate the first (in fact, dummy) tables
    first[0]  = h_hashtable_new(g->arena, h_eq_ptr, h_hash_ptr);
    follow[0] = h_hashtable_new(g->arena, h_eq_ptr, h_hash_ptr);
  }

  // allocate the new tables
  for(size_t i=g->kmax+1; i<=k; i++) {
    first[i]  = h_hashtable_new(g->arena, h_eq_ptr, h_hash_ptr);
    follow[i] = h_hashtable_new(g->arena, h_eq_ptr, h_hash_ptr);
  }

  g->first = first;
  g->follow = follow;
  g->kmax = k;
}

bool h_derives_epsilon(HCFGrammar *g, const HCFChoice *symbol)
{
  // XXX this can now also be implemented in terms of h_first:
  // h_stringmap_present_epsilon(h_first(1, g, symbol))
  // then the geneps structure and associated functions would be unneed

  assert(g->geneps != NULL);

  switch(symbol->type) {
  case HCF_END:       // the end token doesn't count as empty
  case HCF_CHAR:
  case HCF_CHARSET:
    return false;
  default:  // HCF_CHOICE
    return h_hashset_present(g->geneps, symbol);
  }
}

bool h_derives_epsilon_seq(HCFGrammar *g, HCFChoice **s)
{
  // return true iff all symbols in s derive epsilon
  for(; *s; s++) {
    if (!h_derives_epsilon(g, *s)) {
      return false;
    }
  }
  return true;
}

/* Populate the geneps member of g; no-op if called multiple times. */
static void collect_geneps(HCFGrammar *g)
{
  if (g->geneps != NULL) {
    return;
  }

  g->geneps = h_hashset_new(g->arena, h_eq_ptr, h_hash_ptr);
  assert(g->geneps != NULL);

  // iterate over the grammar's symbols, the elements of g->nts.
  // add any we can identify as deriving epsilon to g->geneps.
  // repeat until g->geneps no longer changes.
  size_t prevused;
  do {
    prevused = g->geneps->used;
    size_t i;
    HHashTableEntry *hte;
    for(i=0; i < g->nts->capacity; i++) {
      for(hte = &g->nts->contents[i]; hte; hte = hte->next) {
        if (hte->key == NULL) {
          continue;
	}
        const HCFChoice *symbol = hte->key;
        assert(symbol->type == HCF_CHOICE);

        // this NT derives epsilon if any one of its productions does.
        HCFSequence **p;
        for(p = symbol->seq; *p != NULL; p++) {
          if (h_derives_epsilon_seq(g, (*p)->items)) {
            h_hashset_put(g->geneps, symbol);
            break;
          }
        }
      }
    }
  } while(g->geneps->used != prevused);
}


HStringMap *h_stringmap_new(HArena *a)
{
  HStringMap *m = h_arena_malloc(a, sizeof(HStringMap));
  m->epsilon_branch = NULL;
  m->end_branch = NULL;
  m->char_branches = h_hashtable_new(a, h_eq_ptr, h_hash_ptr);
  m->arena = a;
  return m;
}

void h_stringmap_put_end(HStringMap *m, void *v)
{
  m->end_branch = v;
}

void h_stringmap_put_epsilon(HStringMap *m, void *v)
{
  m->epsilon_branch = v;
}

void h_stringmap_put_after(HStringMap *m, uint8_t c, HStringMap *ends)
{
  h_hashtable_put(m->char_branches, (void *)char_key(c), ends);
}

void h_stringmap_put_char(HStringMap *m, uint8_t c, void *v)
{
  HStringMap *node = h_stringmap_new(m->arena);
  h_stringmap_put_epsilon(node, v);
  h_stringmap_put_after(m, c, node);
}

// helper for h_stringmap_update
static void *combine_stringmap(void *v1, const void *v2)
{
  HStringMap *m1 = v1;
  const HStringMap *m2 = v2;
  if (!m1) {
    m1 = h_stringmap_new(m2->arena);
  }
  h_stringmap_update(m1, m2);

  return m1;
}

/* Note: Does *not* reuse submaps from n in building m. */
void h_stringmap_update(HStringMap *m, const HStringMap *n)
{
  if (n->epsilon_branch) {
    m->epsilon_branch = n->epsilon_branch;
  }
  if (n->end_branch) {
    m->end_branch = n->end_branch;
  }
  h_hashtable_merge(combine_stringmap, m->char_branches, n->char_branches);
}

HStringMap *h_stringmap_copy(HArena *a, const HStringMap *m)
{
  HStringMap *res = h_stringmap_new(a);
  h_stringmap_update(res, m);
  return res;
}

/* Replace all occurances of old in m with new.
 * If old is NULL, replace all values in m with new.
 * If new is NULL, remove the respective values.
 */
void h_stringmap_replace(HStringMap *m, void *old, void *new)
{
  if (!old) {
    if (m->epsilon_branch) {
      m->epsilon_branch = new;
    }
    if (m->end_branch) {
      m->end_branch = new;
    }
  } else {
    if (m->epsilon_branch == old) {
      m->epsilon_branch = new;
    }
    if (m->end_branch == old) {
      m->end_branch = new;
    }
  }

  // iterate over m->char_branches
  const HHashTable *ht = m->char_branches;
  for (size_t i=0; i < ht->capacity; i++) {
    for (HHashTableEntry *hte = &ht->contents[i]; hte; hte = hte->next) {
      if (hte->key == NULL) {
        continue;
      }

      HStringMap *m_ = hte->value;
      if (m_) {
        h_stringmap_replace(m_, old, new);
      }
    }
  }
}

void *h_stringmap_get(const HStringMap *m, const uint8_t *str, size_t n, bool end)
{
  for(size_t i=0; i<n; i++) {
    if (i==n-1 && end && m->end_branch) {
      return m->end_branch;
    }
    m = h_stringmap_get_char(m, str[i]);
    if (!m) {
      return NULL;
    }
  }
  return m->epsilon_branch;
}

void *h_stringmap_get_lookahead(const HStringMap *m, HInputStream lookahead)
{
  while(m) {
    if (m->epsilon_branch) {     // input matched
      // assert: another lookahead would not bring a more specific match.
      //         this is for the table generator to ensure. (LLk)
      return m->epsilon_branch;
    }

    // note the lookahead stream is passed by value, i.e. a copy.
    // reading bits from it does not consume them from the real input.
    uint8_t c = h_read_bits(&lookahead, 8, false);
    
    if (lookahead.overrun) {     // end of input
      // XXX assumption of byte-wise grammar and input
      return m->end_branch;
    }

    // no match yet, descend
    m = h_stringmap_get_char(m, c);
  }

  return NULL;
}

bool h_stringmap_present(const HStringMap *m, const uint8_t *str, size_t n, bool end)
{
  return (h_stringmap_get(m, str, n, end) != NULL);
}

bool h_stringmap_present_epsilon(const HStringMap *m)
{
  return (m->epsilon_branch != NULL);
}

bool h_stringmap_empty(const HStringMap *m)
{
  return (m->epsilon_branch == NULL
          && m->end_branch == NULL
          && h_hashtable_empty(m->char_branches));
}

const HStringMap *h_first(size_t k, HCFGrammar *g, const HCFChoice *x)
{
  HStringMap *ret;
  HCFSequence **p;
  uint8_t c;

  // shortcut: first_0(X) is always {""}
  if (k==0) {
    return g->singleton_epsilon;
  }
  // memoize via g->first
  ensure_k(g, k);
  ret = h_hashtable_get(g->first[k], x);
  if (ret != NULL) {
    return ret;
  }
  ret = h_stringmap_new(g->arena);
  assert(ret != NULL);
  h_hashtable_put(g->first[k], x, ret);

  switch(x->type) {
  case HCF_END:
    h_stringmap_put_end(ret, INSET);
    break;
  case HCF_CHAR:
    h_stringmap_put_char(ret, x->chr, INSET);
    break;
  case HCF_CHARSET:
    c=0;
    do {
      if (charset_isset(x->charset, c)) {
        h_stringmap_put_char(ret, c, INSET);
      }
    } while(c++ < 255);
    break;
  case HCF_CHOICE:
    // this is a nonterminal
    // return the union of the first sets of all productions
    for(p=x->seq; *p; ++p)
      h_stringmap_update(ret, h_first_seq(k, g, (*p)->items));
    break;
  default:  // should not be reached
    assert_message(0, "unknown HCFChoice type");
  }
  
  return ret;
}

// helpers for h_first_seq, definitions below
static bool is_singleton_epsilon(const HStringMap *m);
static bool any_string_shorter(size_t k, const HStringMap *m);

// pointer to functions like h_first_seq
typedef const HStringMap *(*StringSetFun)(size_t, HCFGrammar *, HCFChoice **);

// helper for h_first_seq and h_follow
static void stringset_extend(HCFGrammar *g, HStringMap *ret,
                             size_t k, const HStringMap *as,
                             StringSetFun f, HCFChoice **tail);

const HStringMap *h_first_seq(size_t k, HCFGrammar *g, HCFChoice **s)
{
  // shortcut: the first set of the empty sequence, for any k, is {""}
  if (*s == NULL) {
    return g->singleton_epsilon;
  }
  // first_k(X tail) = { a b | a <- first_k(X), b <- first_l(tail), l=k-|a| }

  HCFChoice *x = s[0];
  HCFChoice **tail = s+1;

  const HStringMap *first_x = h_first(k, g, x);

  // shortcut: if first_k(X) = {""}, just return first_k(tail)
  if (is_singleton_epsilon(first_x)) {
    return h_first_seq(k, g, tail);
  }

  // shortcut: if no elements of first_k(X) have length <k, just return first_k(X)
  if (!any_string_shorter(k, first_x)) {
    return first_x;
  }

  // create a new result set and build up the set described above
  HStringMap *ret = h_stringmap_new(g->arena);

  // extend the elements of first_k(X) up to length k from tail
  stringset_extend(g, ret, k, first_x, h_first_seq, tail);

  return ret;
}

static bool is_singleton_epsilon(const HStringMap *m)
{
  return ( m->epsilon_branch
           && !m->end_branch
           && h_hashtable_empty(m->char_branches) );
}

static bool any_string_shorter(size_t k, const HStringMap *m)
{
  if (k==0) {
    return false;
  }
  if (m->epsilon_branch) {
    return true;
  }
  // iterate over m->char_branches
  const HHashTable *ht = m->char_branches;
  for (size_t i=0; i < ht->capacity; i++) {
    for (HHashTableEntry *hte = &ht->contents[i]; hte; hte = hte->next) {
      if (hte->key == NULL) {
        continue;
      }
      HStringMap *m_ = hte->value;

      // check subtree for strings shorter than k-1
      if (any_string_shorter(k-1, m_)) {
        return true;
      }
    }
  }

  return false;
}

// helper for h_predict
static void remove_all_shorter(size_t k, HStringMap *m)
{
  if (k==0) {
    return;
  }
  m->epsilon_branch = NULL;
  if (k==1) {
    return;
  }

  // iterate over m->char_branches
  const HHashTable *ht = m->char_branches;
  for (size_t i=0; i < ht->capacity; i++) {
    for (HHashTableEntry *hte = &ht->contents[i]; hte; hte = hte->next) {
      if (hte->key == NULL) {
        continue;
      }
      remove_all_shorter(k-1, hte->value);      // recursion into subtree
    }
  }
}

// h_follow adapted to the signature of StringSetFun
static inline
const HStringMap *h_follow_(size_t k, HCFGrammar *g, HCFChoice **s)
{
  return h_follow(k, g, *s);
}

const HStringMap *h_follow(size_t k, HCFGrammar *g, const HCFChoice *x)
{
  // consider all occurances of X in g
  // the follow set of X is the union of:
  //   {$} if X is the start symbol
  //   given a production "A -> alpha X tail":
  //     first_k(tail follow_k(A))

  // first_k(tail follow_k(A)) =
  //   { a b | a <- first_k(tail), b <- follow_l(A), l=k-|a| }

  HStringMap *ret;

  // shortcut: follow_0(X) is always {""}
  if (k==0) {
    return g->singleton_epsilon;
  }
  // memoize via g->follow
  ensure_k(g, k);
  ret = h_hashtable_get(g->follow[k], x);
  if (ret != NULL) {
    return ret;
  }
  ret = h_stringmap_new(g->arena);
  assert(ret != NULL);
  h_hashtable_put(g->follow[k], x, ret);

  // if X is the start symbol, the end token is in its follow set
  if (x == g->start) {
    h_stringmap_put_end(ret, INSET);
  }
  // iterate over g->nts
  size_t i;
  HHashTableEntry *hte;
  for (i=0; i < g->nts->capacity; i++) {
    for (hte = &g->nts->contents[i]; hte; hte = hte->next) {
      if (hte->key == NULL) {
        continue;
      }
      HCFChoice *a = (void *)hte->key;      // production's left-hand symbol
      assert(a->type == HCF_CHOICE);

      // iterate over the productions for A
      HCFSequence **p;
      for (p=a->seq; *p; p++) {
        HCFChoice **s = (*p)->items;        // production's right-hand side
        
        for (; *s; s++) {
          if (*s == x) { // occurance found
            HCFChoice **tail = s+1;

            const HStringMap *first_tail = h_first_seq(k, g, tail);

            // extend the elems of first_k(tail) up to length k from follow(A)
            stringset_extend(g, ret, k, first_tail, h_follow_, &a);
          }
        }
      }
    }
  }

  return ret;
}

HStringMap *h_predict(size_t k, HCFGrammar *g,
                        const HCFChoice *A, const HCFSequence *rhs)
{
  HStringMap *ret = h_stringmap_new(g->arena);

  // predict_k(A -> rhs) =
  //   { ab | a <- first_k(rhs), b <- follow_k(A), |ab|=k }
  
  const HStringMap *first_rhs = h_first_seq(k, g, rhs->items);

  // casting the const off of A below. note: stringset_extend does
  // not touch this argument, only passes it through to h_follow
  // in this case, which accepts it, once again, as const.
  stringset_extend(g, ret, k, first_rhs, h_follow_, (HCFChoice **)&A);

  // make sure there are only strings of length _exactly_ k
  remove_all_shorter(k, ret);

  return ret;
}

// add the set { a b | a <- as, b <- f_l(S), l=k-|a| } to ret
static void stringset_extend(HCFGrammar *g, HStringMap *ret,
                             size_t k, const HStringMap *as,
                             StringSetFun f, HCFChoice **tail)
{
  if (as->epsilon_branch) {
    // for a="", add f_k(tail) to ret
    h_stringmap_update(ret, f(k, g, tail));
  }

  if (as->end_branch) {
    // for a="$", nothing can follow; just add "$" to ret
    // NB: formally, "$" is considered to be of length k
    h_stringmap_put_end(ret, INSET);
  }

  // iterate over as->char_branches
  const HHashTable *ht = as->char_branches;
  for(size_t i=0; i < ht->capacity; i++) {
    for(HHashTableEntry *hte = &ht->contents[i]; hte; hte = hte->next) {
      if (hte->key == NULL) {
        continue;
      }
      uint8_t c = key_char((HCharKey)hte->key);
      
      // follow the branch to find the set { a' | t a' <- as }
      HStringMap *as_ = (HStringMap *)hte->value;

      // now the elements of ret that begin with t are given by
      // t { a b | a <- as_, b <- f_l(tail), l=k-|a|-1 }
      // so we can use recursion over k
      HStringMap *ret_ = h_stringmap_new(g->arena);
      h_stringmap_put_after(ret, c, ret_);

      stringset_extend(g, ret_, k-1, as_, f, tail);
    }
  }
}


void h_pprint_char(FILE *f, char c)
{
  switch(c) {
  case '"': fputs("\\\"", f); break;
  case '\\': fputs("\\\\", f); break;
  case '\b': fputs("\\b", f); break;
  case '\t': fputs("\\t", f); break;
  case '\n': fputs("\\n", f); break;
  case '\r': fputs("\\r", f); break;
  default:
    if (isprint((int)c)) {
      fputc(c, f);
    } else {
      fprintf(f, "\\x%.2X", c);
    }
  }
}

static void pprint_charset_char(FILE *f, char c)
{
  switch(c) {
  case '"': fputc(c, f); break;
  case '-': fputs("\\-", f); break;
  case ']': fputs("\\-", f); break;
  default:  h_pprint_char(f, c);
  }
}

static void pprint_charset(FILE *f, const HCharset cs)
{
  int i;

  fputc('[', f);
  for(i=0; i<256; i++) {
    if (charset_isset(cs, i)) {
      pprint_charset_char(f, i);

      // detect ranges
      if (i+2<256 && charset_isset(cs, i+1) && charset_isset(cs, i+2)) {
        fputc('-', f);
        for(; i<256 && charset_isset(cs, i); i++);
        i--;  // back to the last in range
        pprint_charset_char(f, i);
      }
    }
  }
  fputc(']', f);
}

static const char *nonterminal_name(const HCFGrammar *g, const HCFChoice *nt)
{
  static char buf[16] = {0}; // 14 characters in base 26 are enough for 64 bits

  // find nt's number in g
  size_t n = (uintptr_t)h_hashtable_get(g->nts, nt);

  // NB the start symbol (number 0) is always "A".
  int i;
  for(i=14; i>=0 && (n>0 || i==14); i--) {
    buf[i] = 'A' + n%26;
    n = n/26;   // shift one digit
  }

  return buf+i+1;
}

static HCFChoice **pprint_string(FILE *f, HCFChoice **x)
{
  fputc('"', f);
  for(; *x; x++) {
    if ((*x)->type != HCF_CHAR) {
      break;
    }
    h_pprint_char(f, (*x)->chr);
  }
  fputc('"', f);
  return x;
}

void h_pprint_symbol(FILE *f, const HCFGrammar *g, const HCFChoice *x)
{
  switch(x->type) {
  case HCF_CHAR:
    fputc('"', f);
    h_pprint_char(f, x->chr);
    fputc('"', f);
    break;
  case HCF_END:
    fputc('$', f);
    break;
  case HCF_CHARSET:
    pprint_charset(f, x->charset);
    break;
  default:
    fputs(nonterminal_name(g, x), f);
  }
}

void h_pprint_sequence(FILE *f, const HCFGrammar *g, const HCFSequence *seq)
{
  HCFChoice **x = seq->items;

  if (*x == NULL) {  // the empty sequence
    fputs("\"\"", f);
  } else {
    while(*x) {
      if (x != seq->items) {
	fputc(' ', f); // internal separator
      }
      if ((*x)->type == HCF_CHAR) {
        // condense character strings
        x = pprint_string(f, x);
      } else {
        h_pprint_symbol(f, g, *x);
        x++;
      }
    }
  }
}

// adds some separators expected below
static void pprint_sequence(FILE *f, const HCFGrammar *g, const HCFSequence *seq)
{
  fputc(' ', f);
  h_pprint_sequence(f, g, seq);
  fputc('\n', f);
}

static void pprint_ntrules(FILE *f, const HCFGrammar *g, const HCFChoice *nt,
                           int indent, int len)
{
  int i;
  int column = indent + len;

  const char *name = nonterminal_name(g, nt);

  // print rule head (symbol name)
  for(i=0; i<indent; i++) fputc(' ', f);
  fputs(name, f);
  i += strlen(name);
  for(; i<column; i++) fputc(' ', f);
  fputs(" ->", f);

  assert(nt->type == HCF_CHOICE);
  HCFSequence **p = nt->seq;
  if (*p == NULL) {
    return;          // shouldn't happen
  }
  pprint_sequence(f, g, *p++);    // print first production on the same line
  for(; *p; p++) {                // print the rest below with "or" bars
    for(i=0; i<column; i++) fputc(' ', f);    // indent
    fputs("  |", f);
    pprint_sequence(f, g, *p);
  }
}

void h_pprint_grammar(FILE *file, const HCFGrammar *g, int indent)
{
  if (g->nts->used < 1) {
    return;
  }

  // determine maximum string length of symbol names
  int len;
  size_t s;
  for(len=1, s=26; s < g->nts->used; len++, s*=26); 

  // iterate over g->nts
  size_t i;
  HHashTableEntry *hte;
  for(i=0; i < g->nts->capacity; i++) {
    for(hte = &g->nts->contents[i]; hte; hte = hte->next) {
      if (hte->key == NULL) {
        continue;
      }
      const HCFChoice *a = hte->key;        // production's left-hand symbol
      assert(a->type == HCF_CHOICE);

      pprint_ntrules(file, g, a, indent, len);
    }
  }
}

void h_pprint_symbolset(FILE *file, const HCFGrammar *g, const HHashSet *set, int indent)
{
  int j;
  for(j=0; j<indent; j++) fputc(' ', file);

  fputc('{', file);

  // iterate over set
  size_t i;
  HHashTableEntry *hte;
  const HCFChoice *a = NULL;
  for(i=0; i < set->capacity; i++) {
    for(hte = &set->contents[i]; hte; hte = hte->next) {
      if (hte->key == NULL) {
        continue;
      }
      if(a != NULL) { // we're not on the first element
          fputc(',', file);
      }

      a = hte->key;        // production's left-hand symbol

      h_pprint_symbol(file, g, a);
    }
  }

  fputs("}\n", file);
}

#define BUFSIZE 512

static bool
pprint_stringmap_elems(FILE *file, bool first, char *prefix, size_t n, char sep,
                       void (*valprint)(FILE *f, void *env, void *val), void *env,
                       const HStringMap *map)
{
  assert(n < BUFSIZE-4);

  if (map->epsilon_branch) {
    if (!first) {
      fputc(sep, file); 
      first=false;
    }
    if (n==0) {
      fputs("\"\"", file);
    } else {
      fputs("\"", file);
      fwrite(prefix, 1, n, file);
      fputs("\"", file);
    }

    if (valprint) {
      fputc(':', file);
      valprint(file, env, map->epsilon_branch);
    }
  }

  if (map->end_branch) {
    if (!first) {
      fputs(",\"", file); 
      first=false;
    }
    if (n>0) {
      fputs("\"\"", file);
    }
    fwrite(prefix, 1, n, file);
    if (n>0) {
      fputs("\"\"", file);
    }
    fputs("$", file);

    if (valprint) {
      fputc(':', file);
      valprint(file, env, map->end_branch);
    }
  }

  // iterate over map->char_branches
  HHashTable *ht = map->char_branches;
  size_t i;
  HHashTableEntry *hte;
  for(i=0; i < ht->capacity; i++) {
    for(hte = &ht->contents[i]; hte; hte = hte->next) {
      if (hte->key == NULL) {
        continue;
      }
      uint8_t c = key_char((HCharKey)hte->key);
      HStringMap *ends = hte->value;

      size_t n_ = n;
      switch(c) {
      case '$':  prefix[n_++] = '\\'; prefix[n_++] = '$'; break;
      case '"':  prefix[n_++] = '\\'; prefix[n_++] = '"'; break;
      case '\\': prefix[n_++] = '\\'; prefix[n_++] = '\\'; break;
      case '\b': prefix[n_++] = '\\'; prefix[n_++] = 'b'; break;
      case '\t': prefix[n_++] = '\\'; prefix[n_++] = 't'; break;
      case '\n': prefix[n_++] = '\\'; prefix[n_++] = 'n'; break;
      case '\r': prefix[n_++] = '\\'; prefix[n_++] = 'r'; break;
      default:
        if (isprint(c)) {
          prefix[n_++] = c;
	} else {
          n_ += sprintf(prefix+n_, "\\x%.2X", c);
	}
      }

      first = pprint_stringmap_elems(file, first, prefix, n_,
                                     sep, valprint, env, ends);
    }
  }

  return first;
}

void h_pprint_stringmap(FILE *file, char sep,
                        void (*valprint)(FILE *f, void *env, void *val), void *env,
                        const HStringMap *map)
{
  char buf[BUFSIZE];
  pprint_stringmap_elems(file, true, buf, 0, sep, valprint, env, map);
}

void h_pprint_stringset(FILE *file, const HStringMap *set, int indent)
{
  int j;
  for(j=0; j<indent; j++) fputc(' ', file);

  fputc('{', file);
  h_pprint_stringmap(file, ',', NULL, NULL, set);
  fputs("}\n", file);
}
