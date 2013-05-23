#include "parser_internal.h"

typedef struct {
  const HParser *p1;
  const HParser *p2;
} HTwoParsers;

static HParseResult* parse_difference(void *env, HParseState *state) {
  HTwoParsers *parsers = (HTwoParsers*)env;
  // cache the initial state of the input stream
  HInputStream start_state = state->input_stream;
  HParseResult *r1 = h_do_parse(parsers->p1, state);
  // if p1 failed, bail out early
  if (NULL == r1) {
    return NULL;
  } 
  // cache the state after parse #1, since we might have to back up to it
  HInputStream after_p1_state = state->input_stream;
  state->input_stream = start_state;
  HParseResult *r2 = h_do_parse(parsers->p2, state);
  // TODO(mlp): I'm pretty sure the input stream state should be the post-p1 state in all cases
  state->input_stream = after_p1_state;
  // if p2 failed, restore post-p1 state and bail out early
  if (NULL == r2) {
    return r1;
  }
  size_t r1len = token_length(r1);
  size_t r2len = token_length(r2);
  // if both match but p1's text is shorter than p2's, fail
  if (r1len < r2len) {
    return NULL;
  } else {
    return r1;
  }
}

static HCFChoice* desugar_difference(HAllocator *mm__, void *env) {
  assert_message(0, "'h_difference' is not context-free, can't be desugared");
  return NULL;
}

static HParserVtable difference_vt = {
  .parse = parse_difference,
  .isValidRegular = h_false,
  .isValidCF = h_false, // XXX should this be true if both p1 and p2 are CF?
  .desugar = desugar_difference,
  .compile_to_rvm = h_not_regular,
};

HParser* h_difference(const HParser* p1, const HParser* p2) {
  return h_difference__m(&system_allocator, p1, p2);
}
HParser* h_difference__m(HAllocator* mm__, const HParser* p1, const HParser* p2) { 
  HTwoParsers *env = h_new(HTwoParsers, 1);
  env->p1 = p1;
  env->p2 = p2;
  return h_new_parser(mm__, &difference_vt, env);
}
