#include <assert.h>
#include "parser_internal.h"

static HParseResult* parse_optional(void* env, HParseState* state) {
  HInputStream bak = state->input_stream;
  HParseResult *res0 = h_do_parse((HParser*)env, state);
  if (res0)
    return res0;
  state->input_stream = bak;
  HParsedToken *ast = a_new(HParsedToken, 1);
  ast->token_type = TT_NONE;
  return make_result(state->arena, ast);
}

static bool opt_isValidRegular(void *env) {
  HParser *p = (HParser*) env;
  return p->vtable->isValidRegular(p->env);
}

static bool opt_isValidCF(void *env) {
  HParser *p = (HParser*) env;
  return p->vtable->isValidCF(p->env);
}

static const HParsedToken* reshape_optional(const HParseResult *p) {
  assert(p->ast);
  assert(p->ast->token_type == TT_SEQUENCE);
  assert(p->ast->seq->used > 0);

  HParsedToken *res = p->ast->seq->elements[0];
  if(res)
    return res;

  HParsedToken *ret = h_arena_malloc(p->arena, sizeof(HParsedToken));
  ret->token_type = TT_NONE;
  return ret;
}

static HCFChoice* desugar_optional(HAllocator *mm__, void *env) {
  HParser *p = (HParser*) env;

  /* optional(A) =>
         S  -> A
            -> \epsilon
  */

  HCFChoice *ret = h_new(HCFChoice, 1);
  HCFChoice *a   = h_desugar(mm__, p);
  HCFChoice *eps = desugar_epsilon(mm__, NULL);

  ret->type = HCF_CHOICE;
  ret->seq = h_new(HCFSequence*, 3);  /* enough for 2 productions */

  ret->seq[0] = h_new(HCFSequence, 1);
  ret->seq[0]->items = h_new(HCFChoice*, 2);
  ret->seq[0]->items[0] = a;
  ret->seq[0]->items[1] = NULL;

  ret->seq[1] = h_new(HCFSequence, 1);
  ret->seq[1]->items = h_new(HCFChoice*, 2);
  ret->seq[1]->items[0] = eps;
  ret->seq[1]->items[1] = NULL;

  ret->seq[2] = NULL;

  ret->reshape = reshape_optional;

  return ret;
}

static bool h_svm_action_optional(HArena *arena, HSVMContext *ctx, void *env) {
  if (ctx->stack[ctx->stack_count-1]->token_type == TT_MARK) {
    ctx->stack[ctx->stack_count-1]->token_type = TT_NONE;
  } else {
    ctx->stack_count--;
    assert(ctx->stack[ctx->stack_count-1]->token_type == TT_MARK);
    ctx->stack[ctx->stack_count-1] = ctx->stack[ctx->stack_count];
  }
  return true;
}

static bool opt_ctrvm(HRVMProg *prog, void* env) {
  h_rvm_insert_insn(prog, RVM_PUSH, 0);
  uint16_t insn = h_rvm_insert_insn(prog, RVM_FORK, 0);
  HParser *p = (HParser*) env;
  if (!h_compile_regex(prog, p->env))
    return false;
  h_rvm_patch_arg(prog, insn, h_rvm_get_ip(prog));
  h_rvm_insert_insn(prog, RVM_ACTION, h_rvm_create_action(prog, h_svm_action_optional, NULL));
  return true;
}

static const HParserVtable optional_vt = {
  .parse = parse_optional,
  .isValidRegular = opt_isValidRegular,
  .isValidCF = opt_isValidCF,
  .desugar = desugar_optional,
  .compile_to_rvm = opt_ctrvm,
};

HParser* h_optional(const HParser* p) {
  return h_optional__m(&system_allocator, p);
}
HParser* h_optional__m(HAllocator* mm__, const HParser* p) {
  // TODO: re-add this
  //assert_message(p->vtable != &ignore_vt, "Thou shalt ignore an option, rather than the other way 'round.");
  return h_new_parser(mm__, &optional_vt, (void *)p);
}

