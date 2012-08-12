#include "parser_internal.h"


static HParseResult* parse_charset(void *env, HParseState *state) {
  uint8_t in = h_read_bits(&state->input_stream, 8, false);
  HCharset cs = (HCharset)env;

  if (charset_isset(cs, in)) {
    HParsedToken *tok = a_new(HParsedToken, 1);
    tok->token_type = TT_UINT; tok->uint = in;
    return make_result(state, tok);    
  } else
    return NULL;
}

static const HParserVtable charset_vt = {
  .parse = parse_charset,
};

const HParser* h_ch_range(const uint8_t lower, const uint8_t upper) {
  HParser *ret = g_new(HParser, 1);
  HCharset cs = new_charset();
  for (int i = 0; i < 256; i++)
    charset_set(cs, i, (lower <= i) && (i <= upper));
  ret->vtable = &charset_vt;
  ret->env = (void*)cs;
  return (const HParser*)ret;
}


const HParser* h_in_or_not(const uint8_t *options, size_t count, int val) {
  HParser *ret = g_new(HParser, 1);
  HCharset cs = new_charset();
  for (size_t i = 0; i < 256; i++)
    charset_set(cs, i, 1-val);
  for (size_t i = 0; i < count; i++)
    charset_set(cs, options[i], val);

  ret->vtable = &charset_vt;
  ret->env = (void*)cs;
  return (const HParser*)ret;
}

const HParser* h_in(const uint8_t *options, size_t count) {
  return h_in_or_not(options, count, 1);
}

const HParser* h_not_in(const uint8_t *options, size_t count) {
  return h_in_or_not(options, count, 0);
}

