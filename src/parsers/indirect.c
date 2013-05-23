#include "parser_internal.h"

static HParseResult* parse_indirect(void* env, HParseState* state) {
  return h_do_parse(env, state);
}

static bool indirect_isValidCF(void *env) {
  HParser *p = (HParser*)env;
  return p->vtable->isValidCF(p->env);
}

static HCFChoice* desugar_indirect(HAllocator *mm__, void *env) {
  HParser *p = (HParser*)env;
  return h_desugar(mm__, p);
}

static const HParserVtable indirect_vt = {
  .parse = parse_indirect,
  .isValidRegular = h_false,
  .isValidCF = indirect_isValidCF,
  .desugar = desugar_indirect,
  .compile_to_rvm = h_not_regular,
};

void h_bind_indirect(HParser* indirect, const HParser* inner) {
  assert_message(indirect->vtable == &indirect_vt, "You can only bind an indirect parser");
  indirect->env = (void*)inner;
}

HParser* h_indirect() {
  return h_indirect__m(&system_allocator);
}
HParser* h_indirect__m(HAllocator* mm__) {
  return h_new_parser(mm__, &indirect_vt, NULL);
}
