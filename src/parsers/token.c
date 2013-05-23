#include <assert.h>
#include "parser_internal.h"

typedef struct {
  uint8_t *str;
  uint8_t len;
} HToken;

static HParseResult* parse_token(void *env, HParseState *state) {
  HToken *t = (HToken*)env;
  for (int i=0; i<t->len; ++i) {
    uint8_t chr = (uint8_t)h_read_bits(&state->input_stream, 8, false);
    if (t->str[i] != chr) {
      return NULL;
    }
  }
  HParsedToken *tok = a_new(HParsedToken, 1);
  tok->token_type = TT_BYTES; tok->bytes.token = t->str; tok->bytes.len = t->len;
  return make_result(state->arena, tok);
}


static const HParsedToken *reshape_token(const HParseResult *p) {
  // fetch sequence of uints from p
  assert(p->ast);
  assert(p->ast->token_type == TT_SEQUENCE);
  HCountedArray *seq = p->ast->seq;

  // extract byte string
  uint8_t *arr = h_arena_malloc(p->arena, seq->used);
  size_t i;
  for(i=0; i<seq->used; i++) {
    HParsedToken *t = seq->elements[i];
    assert(t->token_type == TT_UINT);
    arr[i] = t->uint;
  }

  // create result token
  HParsedToken *tok = h_arena_malloc(p->arena, sizeof(HParsedToken));
  tok->token_type = TT_BYTES;
  tok->bytes.len = seq->used;
  tok->bytes.token = arr;

  return tok;
}

static HCFChoice* desugar_token(HAllocator *mm__, void *env) {
  HToken *tok = (HToken*)env;
  HCFSequence *seq = h_new(HCFSequence, 1);
  seq->items = h_new(HCFChoice*, 1+tok->len);
  for (size_t i=0; i<tok->len; ++i) {
    seq->items[i] = h_new(HCFChoice, 1);
    seq->items[i]->type = HCF_CHAR;
    seq->items[i]->chr = tok->str[i];
  }
  seq->items[tok->len] = NULL;
  HCFChoice *ret = h_new(HCFChoice, 1);
  ret->type = HCF_CHOICE;
  ret->seq = h_new(HCFSequence*, 2);
  ret->seq[0] = seq;
  ret->seq[1] = NULL;
  ret->action = NULL;
  ret->pred = NULL;
  ret->reshape = reshape_token;
  return ret;
}

static bool token_ctrvm(HRVMProg *prog, void *env) {
  HToken *t = (HToken*)env;
  h_rvm_insert_insn(prog, RVM_PUSH, 0);
  for (int i=0; i<t->len; ++i) {
    h_rvm_insert_insn(prog, RVM_MATCH, t->str[i] & t->str[i] << 8);
    h_rvm_insert_insn(prog, RVM_STEP, 0);
  }
  h_rvm_insert_insn(prog, RVM_CAPTURE, 0);
  return true;
}

const HParserVtable token_vt = {
  .parse = parse_token,
  .isValidRegular = h_true,
  .isValidCF = h_true,
  .desugar = desugar_token,
  .compile_to_rvm = token_ctrvm,
};

HParser* h_token(const uint8_t *str, const size_t len) {
  return h_token__m(&system_allocator, str, len);
}
HParser* h_token__m(HAllocator* mm__, const uint8_t *str, const size_t len) { 
  HToken *t = h_new(HToken, 1);
  t->str = (uint8_t*)str, t->len = len;
  return h_new_parser(mm__, &token_vt, t);
}
