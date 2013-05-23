#include "parser_internal.h"

static HParseResult* parse_epsilon(void* env, HParseState* state) {
  (void)env;
  HParseResult* res = a_new(HParseResult, 1);
  res->ast = NULL;
  res->arena = state->arena;
  return res;
}

static bool epsilon_ctrvm(HRVMProg *prog, void* env) {
  return true;
}

static const HParserVtable epsilon_vt = {
  .parse = parse_epsilon,
  .isValidRegular = h_true,
  .isValidCF = h_true,
  .desugar = desugar_epsilon,
  .compile_to_rvm = epsilon_ctrvm,
};

HParser* h_epsilon_p() {
  return h_epsilon_p__m(&system_allocator);
}
HParser* h_epsilon_p__m(HAllocator* mm__) {
  HParser *epsilon_p = h_new(HParser, 1);
  epsilon_p->vtable = &epsilon_vt;
  return epsilon_p;
}
