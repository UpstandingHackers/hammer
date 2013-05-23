#include "parser_internal.h"


typedef struct {
  const HParser *p;
  int64_t lower;
  int64_t upper;
} HRange;

static HParseResult* parse_int_range(void *env, HParseState *state) {
  HRange *r_env = (HRange*)env;
  HParseResult *ret = h_do_parse(r_env->p, state);
  if (!ret || !ret->ast)
    return NULL;
  switch(ret->ast->token_type) {
  case TT_SINT:
    if (r_env->lower <= ret->ast->sint && r_env->upper >= ret->ast->sint)
      return ret;
    else
      return NULL;
  case TT_UINT:
    if ((uint64_t)r_env->lower <= ret->ast->uint && (uint64_t)r_env->upper >= ret->ast->uint)
      return ret;
    else
      return NULL;
  default:
    return NULL;
  }
}

HCFChoice* gen_int_range(HAllocator *mm__, uint64_t low, uint64_t high, uint8_t bytes) {
  /* Possible FIXME: TallerThanMe */
  if (1 == bytes) {
    HCFChoice *cs = h_new(HCFChoice, 1);
    cs->type = HCF_CHARSET;
    cs->charset = new_charset(mm__);
    for (uint64_t i=low; i<=high; ++i) {
      charset_set(cs->charset, i, 1);
    }
    cs->action = NULL;
    return cs;
  }
  else if (1 < bytes) {
    uint8_t low_head, hi_head;
    low_head = ((low >> (8*(bytes - 1))) & 0xFF);
    hi_head = ((high >> (8*(bytes - 1))) & 0xFF);
    if (low_head != hi_head) {
      HCFChoice *root = h_new(HCFChoice, 1);
      root->type = HCF_CHOICE;
      root->seq = h_new(HCFSequence*, 4);
      root->seq[0] = h_new(HCFSequence, 1);
      root->seq[0]->items = h_new(HCFChoice*, 3);
      root->seq[0]->items[0] = h_new(HCFChoice, 1);
      root->seq[0]->items[0]->type = HCF_CHAR;
      root->seq[0]->items[0]->chr = low_head;
      root->seq[0]->items[0]->action = NULL;
      root->seq[0]->items[1] = gen_int_range(mm__, low & ((1 << (8 * (bytes - 1))) - 1), ((1 << (8*(bytes-1)))-1), bytes-1);
      root->seq[0]->items[2] = NULL;
      root->seq[1] = h_new(HCFSequence, 1);
      root->seq[1]->items = h_new(HCFChoice*, bytes+1);
      root->seq[1]->items[0] = h_new(HCFChoice, 2);
      root->seq[1]->items[0]->type = HCF_CHARSET;
      root->seq[1]->items[0]->charset = new_charset(mm__);
      root->seq[1]->items[0]->action = NULL;
      root->seq[1]->items[1] = root->seq[1]->items[0] + 1;
      root->seq[1]->items[1]->type = HCF_CHARSET;
      root->seq[1]->items[1]->charset = new_charset(mm__);
      for (int i = 0; i < 256; i++) {
	charset_set(root->seq[1]->items[0]->charset, i, (i > low_head && i < hi_head));
	charset_set(root->seq[1]->items[1]->charset, i, 1);
      }
      root->seq[1]->items[1]->action = NULL;
      for (int i = 2; i < bytes; i++)
	root->seq[1]->items[i] = root->seq[1]->items[1];
      root->seq[1]->items[bytes] = NULL;
      root->seq[2] = h_new(HCFSequence, 1);
      root->seq[2]->items = h_new(HCFChoice*, 3);
      root->seq[2]->items[0] = h_new(HCFChoice, 1);
      root->seq[2]->items[0]->type = HCF_CHAR;
      root->seq[2]->items[0]->type = hi_head;
      root->seq[2]->items[0]->action = NULL;
      root->seq[2]->items[1] = gen_int_range(mm__, 0, high & ((1 << (8 * (bytes - 1))) - 1), bytes-1);
      root->seq[2]->items[2] = NULL;
      root->seq[3] = NULL;
      root->action = NULL;
      return root;
    } else {
      HCFChoice *root = h_new(HCFChoice, 1);
      root->type = HCF_CHOICE;
      root->seq = h_new(HCFSequence*, 2);
      root->seq[0] = h_new(HCFSequence, 1);
      root->seq[0]->items = h_new(HCFChoice*, 3);
      root->seq[0]->items[0] = h_new(HCFChoice, 1);
      root->seq[0]->items[0]->type = HCF_CHAR;
      root->seq[0]->items[0]->chr = low_head;
      root->seq[0]->items[0]->action = NULL;
      root->seq[0]->items[1] = gen_int_range(mm__,
					     low & ((1 << (8 * (bytes - 1))) - 1),
					     high & ((1 << (8 * (bytes - 1))) - 1),
					     bytes - 1);
      root->seq[0]->items[2] = NULL;
      root->seq[1] = NULL;
      root->action = NULL;
      return root;
    }
  }
  else { // idk why this would ever be <1, but whatever
    return NULL; 
  }
}

struct bits_env {
  uint8_t length;
  uint8_t signedp;
};

static HCFChoice* desugar_int_range(HAllocator *mm__, void *env) {
  HRange *r = (HRange*)env;
  struct bits_env* be = (struct bits_env*)r->p->env;
  uint8_t bytes = be->length / 8;
  return gen_int_range(mm__, r->lower, r->upper, bytes);
}

bool h_svm_action_validate_int_range(HArena *arena, HSVMContext *ctx, void* env) {
  HRange *r_env = (HRange*)env;
  HParsedToken *head = ctx->stack[ctx->stack_count-1];
  switch (head-> token_type) {
  case TT_SINT: 
    return head->sint >= r_env->lower && head->sint <= r_env->upper;
  case TT_UINT: 
    return head->uint >= (uint64_t)r_env->lower && head->uint <= (uint64_t)r_env->upper;
  default:
    return false;
  }
}

static bool ir_ctrvm(HRVMProg *prog, void *env) {
  HRange *r_env = (HRange*)env;
  
  h_compile_regex(prog, r_env->p);
  h_rvm_insert_insn(prog, RVM_ACTION, h_rvm_create_action(prog, h_svm_action_validate_int_range, env));
  return false;
}

static const HParserVtable int_range_vt = {
  .parse = parse_int_range,
  .isValidRegular = h_true,
  .isValidCF = h_true,
  .desugar = desugar_int_range,
  .compile_to_rvm = ir_ctrvm,
};

HParser* h_int_range(const HParser *p, const int64_t lower, const int64_t upper) {
  return h_int_range__m(&system_allocator, p, lower, upper);
}
HParser* h_int_range__m(HAllocator* mm__, const HParser *p, const int64_t lower, const int64_t upper) {
  // p must be an integer parser, which means it's using parse_bits
  // TODO: re-add this check
  //assert_message(p->vtable == &bits_vt, "int_range requires an integer parser"); 

  // and regardless, the bounds need to fit in the parser in question
  // TODO: check this as well.

  HRange *r_env = h_new(HRange, 1);
  r_env->p = p;
  r_env->lower = lower;
  r_env->upper = upper;
  return h_new_parser(mm__, &int_range_vt, r_env);
}
