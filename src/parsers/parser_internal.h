#ifndef HAMMER_PARSE_INTERNAL__H
#define HAMMER_PARSE_INTERNAL__H
#include "../hammer.h"
#include "../internal.h"
#include "../backends/regex.h"

#define a_new_(arena, typ, count) ((typ*)h_arena_malloc((arena), sizeof(typ)*(count)))
#define a_new(typ, count) a_new_(state->arena, typ, count)
// we can create a_new0 if necessary. It would allocate some memory and immediately zero it out.

static inline HParseResult* make_result(HArena *arena, HParsedToken *tok) {
  HParseResult *ret = h_arena_malloc(arena, sizeof(HParseResult));
  ret->ast = tok;
  ret->arena = arena;
  return ret;
}

// return token size in bits...
static inline size_t token_length(HParseResult *pr) {
  if (pr) {
    return pr->bit_length;
  } else {
    return 0;
  }
}

/* Epsilon rules happen during desugaring. This handles them. */
static inline HCFChoice* desugar_epsilon(HAllocator *mm__, void *env) {
  static HCFChoice *res_seq_l[] = {NULL};
  static HCFSequence res_seq = {res_seq_l};
  static HCFSequence *res_ch_l[] = {&res_seq, NULL};
  static HCFChoice res_ch = {
    .type = HCF_CHOICE,
    .seq = res_ch_l,
    .action = NULL,
    .reshape = h_act_ignore
  };
  return &res_ch;
}

#endif // HAMMER_PARSE_INTERNAL__H
