#include "parser_internal.h"

static HParseResult* parse_not(void* env, HParseState* state) {
  HInputStream bak = state->input_stream;
  if (h_do_parse((HParser*)env, state))
    return NULL;
  else {
    state->input_stream = bak;
    return make_result(state->arena, NULL);
  }
}

static HCFChoice* desugar_not(HAllocator *mm__, void *env) {
  assert_message(0, "'h_not' is not context-free, can't be desugared");
  return NULL;
}

static const HParserVtable not_vt = {
  .parse = parse_not,
  .isValidRegular = h_false,  /* see and.c for why */
  .isValidCF = h_false,       /* also see and.c for why */
  .desugar = desugar_not,
  .compile_to_rvm = h_not_regular, // Is actually regular, but the generation step is currently unable to handle it. TODO: fix this.
};

HParser* h_not(const HParser* p) {
  return h_not__m(&system_allocator, p);
}
HParser* h_not__m(HAllocator* mm__, const HParser* p) {
  return h_new_parser(mm__, &not_vt, (void *)p);
}
