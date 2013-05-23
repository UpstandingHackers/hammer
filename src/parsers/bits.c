#include <assert.h>
#include "parser_internal.h"

struct bits_env {
  uint8_t length;
  uint8_t signedp;
};

static HParseResult* parse_bits(void* env, HParseState *state) {
  struct bits_env *env_ = env;
  HParsedToken *result = a_new(HParsedToken, 1);
  result->token_type = (env_->signedp ? TT_SINT : TT_UINT);
  if (env_->signedp)
    result->sint = h_read_bits(&state->input_stream, env_->length, true);
  else
    result->uint = h_read_bits(&state->input_stream, env_->length, false);
  return make_result(state->arena, result);
}

static HParsedToken *reshape_bits(const HParseResult *p, bool signedp) {
  // XXX works only for whole bytes
  // XXX assumes big-endian
  assert(p->ast);
  assert(p->ast->token_type == TT_SEQUENCE);

  HCountedArray *seq = p->ast->seq;
  HParsedToken *ret = h_arena_malloc(p->arena, sizeof(HParsedToken));
  ret->token_type = TT_UINT;

  if(signedp && (seq->elements[0]->uint & 128))
    ret->uint = -1; // all ones

  for(size_t i=0; i<seq->used; i++) {
    HParsedToken *t = seq->elements[i];
    assert(t->token_type == TT_UINT);

    ret->uint <<= 8;
    ret->uint |= t->uint & 0xFF;
  }

  if(signedp) {
    ret->token_type = TT_SINT;
    ret->sint = ret->uint;
  }

  return ret;
}
static const HParsedToken *reshape_bits_unsigned(const HParseResult *p) {
  return reshape_bits(p, false);
}
static const HParsedToken *reshape_bits_signed(const HParseResult *p) {
  return reshape_bits(p, true);
}

static HCFChoice* desugar_bits(HAllocator *mm__, void *env) {
  struct bits_env *bits = (struct bits_env*)env;
  if (0 != bits->length % 8)
    return NULL; // can't handle non-byte-aligned for now

  HCharset match_all = new_charset(mm__);
  for (int i = 0; i < 256; i++)
    charset_set(match_all, i, 1);

  HCFChoice *match_all_choice = h_new(HCFChoice, 1);
  match_all_choice->type = HCF_CHARSET;
  match_all_choice->charset = match_all;
  match_all_choice->action = NULL;

  size_t n = bits->length/8;
  HCFSequence *seq = h_new(HCFSequence, 1);
  seq->items = h_new(HCFChoice*, n+1);
  for (size_t i=0; i<n; ++i) {
    seq->items[i] = match_all_choice;
  }
  seq->items[n] = NULL;

  HCFChoice *ret = h_new(HCFChoice, 1);
  ret->type = HCF_CHOICE;
  ret->seq = h_new(HCFSequence*, 2);
  ret->seq[0] = seq;
  ret->seq[1] = NULL;
  ret->action = NULL;

  if(bits->signedp)
    ret->reshape = reshape_bits_signed;
  else
    ret->reshape = reshape_bits_unsigned;

  return ret;
}

static bool h_svm_action_bits(HArena *arena, HSVMContext *ctx, void* env) {
  // BUG: relies un undefined behaviour: int64_t is a signed uint64_t; not necessarily true on 32-bit
  struct bits_env *env_ = env;
  HParsedToken *top = ctx->stack[ctx->stack_count-1];
  assert(top->token_type == TT_BYTES);
  uint64_t res = 0;
  for (size_t i = 0; i < top->bytes.len; i++)
    res = (res << 8) | top->bytes.token[i];   // TODO: Handle other endiannesses.
  top->uint = res; // possibly cast to signed through union
  top->token_type = (env_->signedp ? TT_SINT : TT_UINT);
  return true;
}

static bool bits_ctrvm(HRVMProg *prog, void* env) {
  struct bits_env *env_ = (struct bits_env*)env;
  h_rvm_insert_insn(prog, RVM_PUSH, 0);
  for (size_t i=0; (i < env_->length)/8; ++i) { // FUTURE: when we can handle non-byte-aligned, the env_->length/8 part will be different
    h_rvm_insert_insn(prog, RVM_MATCH, 0xFF00);
    h_rvm_insert_insn(prog, RVM_STEP, 0);
  }
  h_rvm_insert_insn(prog, RVM_CAPTURE, 0);
  h_rvm_insert_insn(prog, RVM_ACTION, h_rvm_create_action(prog, h_svm_action_bits, env));
  return true;
}

static const HParserVtable bits_vt = {
  .parse = parse_bits,
  .isValidRegular = h_true,
  .isValidCF = h_true,
  .desugar = desugar_bits,
  .compile_to_rvm = bits_ctrvm,
};

HParser* h_bits(size_t len, bool sign) {
  return h_bits__m(&system_allocator, len, sign);
}
HParser* h_bits__m(HAllocator* mm__, size_t len, bool sign) {
  struct bits_env *env = h_new(struct bits_env, 1);
  env->length = len;
  env->signedp = sign;
  return h_new_parser(mm__, &bits_vt, env);
}

#define SIZED_BITS(name_pre, len, signedp) \
  HParser* h_##name_pre##len () {				\
    return h_bits__m(&system_allocator, len, signedp);		\
  }								\
  HParser* h_##name_pre##len##__m(HAllocator* mm__) {	\
    return h_bits__m(mm__, len, signedp);			\
  }
SIZED_BITS(int, 8, true)
SIZED_BITS(int, 16, true)
SIZED_BITS(int, 32, true)
SIZED_BITS(int, 64, true)
SIZED_BITS(uint, 8, false)
SIZED_BITS(uint, 16, false)
SIZED_BITS(uint, 32, false)
SIZED_BITS(uint, 64, false)
