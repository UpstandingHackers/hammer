#include <ctype.h>
#include "parser_internal.h"

static HParseResult* parse_whitespace(void* env, HParseState *state) {
  char c;
  HInputStream bak;
  do {
    bak = state->input_stream;
    c = h_read_bits(&state->input_stream, 8, false);
    if (state->input_stream.overrun)
      break;
  } while (isspace(c));
  state->input_stream = bak;
  return h_do_parse((HParser*)env, state);
}

static const HParserVtable whitespace_vt = {
  .parse = parse_whitespace,
};

const HParser* h_whitespace(const HParser* p) {
  return h_whitespace__m(&system_allocator, p);
}
const HParser* h_whitespace__m(HAllocator* mm__, const HParser* p) {
  HParser *ret = h_new(HParser, 1);
  ret->vtable = &whitespace_vt;
  ret->env = (void*)p;
  return ret;
}
