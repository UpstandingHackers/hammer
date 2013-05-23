/* Context-free grammar representation and analysis */

#include "internal.h"


typedef struct HCFGrammar_ {
  HCFChoice   *start;   // start symbol (nonterminal)
  HHashSet    *nts;     // HCFChoices, each representing the alternative
                        // productions for one nonterminal
  HHashSet    *geneps;  // set of NTs that can generate the empty string
  HHashTable  **first;  // memoized first sets of the grammar's symbols
  HHashTable  **follow; // memoized follow sets of the grammar's NTs
  size_t      kmax;     // maximum lookahead depth allocated
  HArena      *arena;
  HAllocator  *mm__;

  // constant set containing only the empty string.
  // this is only a member of HCFGrammar because it needs a pointer to arena.
  const struct HCFStringMap_ *singleton_epsilon;
} HCFGrammar;


/* Representing input characters (bytes) in HHashTables.
 * To use these as keys, we must avoid 0 as because NULL means "not set".
 */
typedef uintptr_t HCharKey;
static inline HCharKey char_key(uint8_t c) { return (0x100 | c); }
static inline uint8_t key_char(HCharKey k) { return (0xFF & k); }

/* Mapping strings of input tokens to arbitrary values (or serving as a set).
 * Common prefixes are folded into a tree of HHashTables, branches labeled with
 * input tokens.
 * Each path through the tree represents the string along its branches.
 */
typedef struct HCFStringMap_ {
  void *epsilon_branch;         // points to leaf value
  void *end_branch;             // points to leaf value
  HHashTable *char_branches;    // maps to inner nodes (HCFStringMaps)
  HArena *arena;
} HCFStringMap;

HCFStringMap *h_stringmap_new(HArena *a);
void h_stringmap_put_end(HCFStringMap *m, void *v);
void h_stringmap_put_epsilon(HCFStringMap *m, void *v);
void h_stringmap_put_after(HCFStringMap *m, uint8_t c, HCFStringMap *ends);
void h_stringmap_put_char(HCFStringMap *m, uint8_t c, void *v);
void h_stringmap_update(HCFStringMap *m, const HCFStringMap *n);
void h_stringmap_replace(HCFStringMap *m, void *old, void *new);
void *h_stringmap_get(const HCFStringMap *m, const uint8_t *str, size_t n, bool end);
bool h_stringmap_present(const HCFStringMap *m, const uint8_t *str, size_t n, bool end);
bool h_stringmap_present_epsilon(const HCFStringMap *m);

static inline void *h_stringmap_get_char(const HCFStringMap *m, const uint8_t c)
 { return h_hashtable_get(m->char_branches, (void *)char_key(c)); }


/* Convert 'parser' into CFG representation by desugaring and compiling the set
 * of nonterminals.
 * A NULL return means we are unable to represent the parser as a CFG.
 */
HCFGrammar *h_cfgrammar(HAllocator* mm__, const HParser *parser);

/* Frees the given grammar and associated data.
 * Does *not* free parsers' CFG forms as created by h_desugar.
 */
void h_cfgrammar_free(HCFGrammar *g);

/* Does the given symbol derive the empty string (under g)? */
bool h_derives_epsilon(HCFGrammar *g, const HCFChoice *symbol);

/* Does the sentential form s derive the empty string? s NULL-terminated. */
bool h_derives_epsilon_seq(HCFGrammar *g, HCFChoice **s);

/* Compute first_k set of symbol x. Memoized. */
const HCFStringMap *h_first(size_t k, HCFGrammar *g, const HCFChoice *x);

/* Compute first_k set of sentential form s. s NULL-terminated. */
const HCFStringMap *h_first_seq(size_t k, HCFGrammar *g, HCFChoice **s);

/* Compute follow_k set of symbol x. Memoized. */
const HCFStringMap *h_follow(size_t k, HCFGrammar *g, const HCFChoice *x);


/* Pretty-printers for grammars and associated data. */
void h_pprint_grammar(FILE *file, const HCFGrammar *g, int indent);
void h_pprint_symbolset(FILE *file, const HCFGrammar *g, const HHashSet *set, int indent);
void h_pprint_stringset(FILE *file, const HCFGrammar *g, const HCFStringMap *set, int indent);
