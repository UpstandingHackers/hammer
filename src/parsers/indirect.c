#include "parser_internal.h"

static HParseResult* parse_indirect(void* env, HParseState* state) {
  return h_do_parse(env, state);
}
static const HParserVtable indirect_vt = {
  .parse = parse_indirect,
};

void h_bind_indirect(HParser* indirect, const HParser* inner) {
  assert_message(indirect->vtable == &indirect_vt, "You can only bind an indirect parser");
  indirect->env = (void*)inner;
}

HParser* h_indirect() {
  HParser *res = g_new(HParser, 1);
  res->vtable = &indirect_vt;
  res->env = NULL;
  return res;
}
