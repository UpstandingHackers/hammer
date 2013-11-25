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


static HParsedToken *reshape_token(const HParseResult *p, void* user_data) {
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

static void desugar_token(HAllocator *mm__, HCFStack *stk__, void *env) {
  HToken *tok = (HToken*)env;
  HCFS_BEGIN_CHOICE() {
    HCFS_BEGIN_SEQ() {
      for (size_t i = 0; i < tok->len; i++)
	HCFS_ADD_CHAR(tok->str[i]);
    } HCFS_END_SEQ();
    HCFS_THIS_CHOICE->reshape = reshape_token;
    HCFS_THIS_CHOICE->user_data = NULL;
  } HCFS_END_CHOICE();
}

static bool token_ctrvm(HRVMProg *prog, void *env) {
  HToken *t = (HToken*)env;
  h_rvm_insert_insn(prog, RVM_PUSH, 0);
  for (int i=0; i<t->len; ++i) {
    h_rvm_insert_insn(prog, RVM_MATCH, t->str[i] | t->str[i] << 8);
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
  uint8_t *str_cpy = h_new(uint8_t, len);
  memcpy(str_cpy, str, len);
  t->str = str_cpy, t->len = len;
  return h_new_parser(mm__, &token_vt, t);
}
