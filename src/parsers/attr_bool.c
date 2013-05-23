#include "parser_internal.h"

typedef struct {
  const HParser *p;
  HPredicate pred;
} HAttrBool;

static HParseResult* parse_attr_bool(void *env, HParseState *state) {
  HAttrBool *a = (HAttrBool*)env;
  HParseResult *res = h_do_parse(a->p, state);
  if (res && res->ast) {
    if (a->pred(res))
      return res;
    else
      return NULL;
  } else
    return NULL;
}

static bool ab_isValidRegular(void *env) {
  HAttrBool *ab = (HAttrBool*)env;
  return ab->p->vtable->isValidRegular(ab->p->env);
}

/* Strictly speaking, this does not adhere to the definition of context-free.
   However, for the purpose of making it easier to handle weird constraints on
   fields, we've decided to allow boolean attribute validation at parse time
   for now. We might change our minds later. */

static bool ab_isValidCF(void *env) {
  HAttrBool *ab = (HAttrBool*)env;
  return ab->p->vtable->isValidCF(ab->p->env);
}

static HCFChoice* desugar_ab(HAllocator *mm__, void *env) {
  HAttrBool *a = (HAttrBool*)env;
  HCFSequence *seq = h_new(HCFSequence, 1);
  seq->items = h_new(HCFChoice*, 2);
  seq->items[0] = h_desugar(mm__, a->p);
  seq->items[1] = NULL;
  HCFChoice *ret = h_new(HCFChoice, 1);
  ret->type = HCF_CHOICE;
  ret->seq = h_new(HCFSequence*, 2);
  ret->seq[0] = seq;
  ret->seq[1] = NULL;
  ret->pred = a->pred;
  ret->reshape = h_act_first;
  return ret;
}

static bool ab_ctrvm(HRVMProg *prog, void *env) {
  HAttrBool *ab = (HAttrBool*)env;
  return h_compile_regex(prog, ab->p);
}

static const HParserVtable attr_bool_vt = {
  .parse = parse_attr_bool,
  .isValidRegular = ab_isValidRegular,
  .isValidCF = ab_isValidCF,
  .desugar = desugar_ab,
  .compile_to_rvm = ab_ctrvm,
};


HParser* h_attr_bool(const HParser* p, HPredicate pred) {
  return h_attr_bool__m(&system_allocator, p, pred);
}
HParser* h_attr_bool__m(HAllocator* mm__, const HParser* p, HPredicate pred) { 
  HAttrBool *env = h_new(HAttrBool, 1);
  env->p = p;
  env->pred = pred;
  return h_new_parser(mm__, &attr_bool_vt, env);
}
