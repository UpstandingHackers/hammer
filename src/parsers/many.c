#include "parser_internal.h"

// TODO: split this up.
typedef struct {
  const HParser *p, *sep;
  size_t count;
  bool min_p;
} HRepeat;

static HParseResult *parse_many(void* env, HParseState *state) {
  HRepeat *env_ = (HRepeat*) env;
  HCountedArray *seq = h_carray_new_sized(state->arena, (env_->count > 0 ? env_->count : 4));
  size_t count = 0;
  HInputStream bak;
  while (env_->min_p || env_->count > count) {
    bak = state->input_stream;
    if (count > 0) {
      HParseResult *sep = h_do_parse(env_->sep, state);
      if (!sep)
	goto err0;
    }
    HParseResult *elem = h_do_parse(env_->p, state);
    if (!elem)
      goto err0;
    if (elem->ast)
      h_carray_append(seq, (void*)elem->ast);
    count++;
  }
  if (count < env_->count)
    goto err;
 succ:
  ; // necessary for the label to be here...
  HParsedToken *res = a_new(HParsedToken, 1);
  res->token_type = TT_SEQUENCE;
  res->seq = seq;
  return make_result(state->arena, res);
 err0:
  if (count >= env_->count) {
    state->input_stream = bak;
    goto succ;
  }
 err:
  state->input_stream = bak;
  return NULL;
}

static bool many_isValidRegular(void *env) {
  HRepeat *repeat = (HRepeat*)env;
  return (repeat->p->vtable->isValidRegular(repeat->p->env) &&
	  repeat->sep->vtable->isValidRegular(repeat->sep->env));
}

static bool many_isValidCF(void *env) {
  HRepeat *repeat = (HRepeat*)env;
  return (repeat->p->vtable->isValidCF(repeat->p->env) &&
	  repeat->sep->vtable->isValidCF(repeat->sep->env));
}

static HCFChoice* desugar_many(HAllocator *mm__, void *env) {
  HRepeat *repeat = (HRepeat*)env;
  if(repeat->count > 1) {
    assert_message(0, "'h_repeat_n' is not context-free, can't be desugared");
    return NULL;
  }

  /* many(A) =>
         Ma  -> A Mar
             -> \epsilon (but not if many1/sepBy1 is used) 
         Mar -> Sep A Mar
             -> \epsilon
  */

  HCFChoice *sep = h_desugar(mm__, repeat->sep);
  HCFChoice *a   = h_desugar(mm__, repeat->p);
  HCFChoice *ma  = h_new(HCFChoice, 1);
  HCFChoice *mar = h_new(HCFChoice, 1);
  HCFChoice *eps = desugar_epsilon(mm__, NULL);

  /* create first subrule */
  ma->type = HCF_CHOICE;
  ma->seq = h_new(HCFSequence*, 3);  /* enough for 2 productions */
  ma->seq[0] = h_new(HCFSequence, 1);
  ma->seq[0]->items = h_new(HCFChoice*, 3);
  ma->seq[0]->items[0] = a;
  ma->seq[0]->items[1] = mar;
  ma->seq[0]->items[2] = NULL;
  ma->seq[1] = NULL;

  /* if not many1/sepBy1, attach epsilon */
  if (repeat->count == 0) {
    ma->seq[1] = h_new(HCFSequence, 1);
    ma->seq[1]->items = h_new(HCFChoice*, 2);
    ma->seq[1]->items[0] = eps;
    ma->seq[1]->items[1] = NULL;
    ma->seq[2] = NULL;
  }

  /* create second subrule */
  mar->type = HCF_CHOICE;
  mar->seq = h_new(HCFSequence*, 3);
  mar->seq[0] = h_new(HCFSequence, 1);
  mar->seq[0]->items = h_new(HCFChoice*, 4);
  mar->seq[0]->items[0] = sep;
  mar->seq[0]->items[1] = a;
  mar->seq[0]->items[2] = mar; // woo recursion!
  mar->seq[0]->items[3] = NULL;
  mar->seq[1] = h_new(HCFSequence, 1);
  mar->seq[1]->items = h_new(HCFChoice*, 2);
  mar->seq[1]->items[0] = eps;
  mar->seq[1]->items[1] = NULL;
  mar->seq[2] = NULL;

  /* attach reshapers */
  sep->reshape = h_act_ignore; 
  ma->reshape = h_act_flatten;

  return ma;
}

static bool many_ctrvm(HRVMProg *prog, void *env) {
  HRepeat *repeat = (HRepeat*)env;
  // FIXME: Implement clear_to_mark
  uint16_t clear_to_mark = h_rvm_create_action(prog, h_svm_action_clear_to_mark, NULL);
  h_rvm_insert_insn(prog, RVM_PUSH, 0);
  // TODO: implement min and max properly. Right now, it's always min==0, max==inf
  uint16_t insn = h_rvm_insert_insn(prog, RVM_FORK, 0);
  if (!h_compile_regex(prog, repeat->p))
    return false;
  if (repeat->sep != NULL) {
    h_rvm_insert_insn(prog, RVM_PUSH, 0);
    if (!h_compile_regex(prog, repeat->sep))
      return false;
    h_rvm_insert_insn(prog, RVM_ACTION, clear_to_mark);
  }
  h_rvm_insert_insn(prog, RVM_GOTO, insn);
  h_rvm_patch_arg(prog, insn, h_rvm_get_ip(prog));

  h_rvm_insert_insn(prog, RVM_ACTION, h_rvm_create_action(prog, h_svm_action_make_sequence, NULL));
  return true;
}

static const HParserVtable many_vt = {
  .parse = parse_many,
  .isValidRegular = many_isValidRegular,
  .isValidCF = many_isValidCF,
  .desugar = desugar_many,
  .compile_to_rvm = many_ctrvm,
};

HParser* h_many(const HParser* p) {
  return h_many__m(&system_allocator, p);
}
HParser* h_many__m(HAllocator* mm__, const HParser* p) {
  HRepeat *env = h_new(HRepeat, 1);
  env->p = p;
  env->sep = h_epsilon_p__m(mm__);
  env->count = 0;
  env->min_p = true;
  return h_new_parser(mm__, &many_vt, env);
}

HParser* h_many1(const HParser* p) {
  return h_many1__m(&system_allocator, p);
}
HParser* h_many1__m(HAllocator* mm__, const HParser* p) {
  HRepeat *env = h_new(HRepeat, 1);
  env->p = p;
  env->sep = h_epsilon_p__m(mm__);
  env->count = 1;
  env->min_p = true;
  return h_new_parser(mm__, &many_vt, env);
}

HParser* h_repeat_n(const HParser* p, const size_t n) {
  return h_repeat_n__m(&system_allocator, p, n);
}
HParser* h_repeat_n__m(HAllocator* mm__, const HParser* p, const size_t n) {
  HRepeat *env = h_new(HRepeat, 1);
  env->p = p;
  env->sep = h_epsilon_p__m(mm__);
  env->count = n;
  env->min_p = false;
  return h_new_parser(mm__, &many_vt, env);
}

HParser* h_sepBy(const HParser* p, const HParser* sep) {
  return h_sepBy__m(&system_allocator, p, sep);
}
HParser* h_sepBy__m(HAllocator* mm__, const HParser* p, const HParser* sep) {
  HRepeat *env = h_new(HRepeat, 1);
  env->p = p;
  env->sep = sep;
  env->count = 0;
  env->min_p = true;
  return h_new_parser(mm__, &many_vt, env);
}

HParser* h_sepBy1(const HParser* p, const HParser* sep) {
  return h_sepBy1__m(&system_allocator, p, sep);
}
HParser* h_sepBy1__m(HAllocator* mm__, const HParser* p, const HParser* sep) {
  HRepeat *env = h_new(HRepeat, 1);
  env->p = p;
  env->sep = sep;
  env->count = 1;
  env->min_p = true;
  return h_new_parser(mm__, &many_vt, env);
}

typedef struct {
  const HParser *length;
  const HParser *value;
} HLenVal;

static HParseResult* parse_length_value(void *env, HParseState *state) {
  HLenVal *lv = (HLenVal*)env;
  HParseResult *len = h_do_parse(lv->length, state);
  if (!len)
    return NULL;
  if (len->ast->token_type != TT_UINT)
    errx(1, "Length parser must return an unsigned integer");
  // TODO: allocate this using public functions
  HRepeat repeat = {
    .p = lv->value,
    .sep = h_epsilon_p(),
    .count = len->ast->uint,
    .min_p = false
  };
  return parse_many(&repeat, state);
}

static HCFChoice* desugar_length_value(HAllocator *mm__, void *env) {
  assert_message(0, "'h_length_value' is not context-free, can't be desugared");
  return NULL;
}

static const HParserVtable length_value_vt = {
  .parse = parse_length_value,
  .isValidRegular = h_false,
  .isValidCF = h_false,
  .desugar = desugar_length_value,
};

HParser* h_length_value(const HParser* length, const HParser* value) {
  return h_length_value__m(&system_allocator, length, value);
}
HParser* h_length_value__m(HAllocator* mm__, const HParser* length, const HParser* value) {
  HLenVal *env = h_new(HLenVal, 1);
  env->length = length;
  env->value = value;
  return h_new_parser(mm__, &length_value_vt, env);
}
