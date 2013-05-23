#include <assert.h>
#include "parser_internal.h"


// XXX I'm thinking these parsers should be removed entirely in favor of an
//     equivalent family of HActions.  --pesco


//
// general case: parse sequence, pick one result
//

typedef struct HIgnoreSeq_ {
  const HParser **parsers;
  size_t len;         // how many parsers in 'ps'
  size_t which;         // whose result to return
} HIgnoreSeq;

static HParseResult* parse_ignoreseq(void* env, HParseState *state) {
  const HIgnoreSeq *seq = (HIgnoreSeq*)env;
  HParseResult *res = NULL;

  for (size_t i=0; i < seq->len; ++i) {
    HParseResult *tmp = h_do_parse(seq->parsers[i], state);
    if (!tmp)
      return NULL;
    else if (i == seq->which)
      res = tmp;
  }

  return res;
}

static HCFChoice* desugar_ignoreseq(HAllocator *mm__, void *env) {
  HIgnoreSeq *seq = (HIgnoreSeq*)env;
  HCFSequence *hseq = h_new(HCFSequence, 1);
  hseq->items = h_new(HCFChoice*, 1+seq->len);
  for (size_t i=0; i<seq->len; ++i) {
    hseq->items[i] = h_desugar(mm__, seq->parsers[i]);
  }
  hseq->items[seq->len] = NULL;
  HCFChoice *ret = h_new(HCFChoice, 1);
  ret->type = HCF_CHOICE;
  ret->seq = h_new(HCFSequence*, 2);
  ret->seq[0] = hseq;
  ret->seq[1] = NULL;
  ret->action = NULL;

  if(seq->which == 0)
    ret->reshape = h_act_first;
  else if(seq->which == 1)
    ret->reshape = h_act_second;    // for h_middle
  else if(seq->which == seq->len-1)
    ret->reshape = h_act_last;
  else
    ret->reshape = NULL;    // XXX

  return ret;
}

static bool is_isValidRegular(void *env) {
  HIgnoreSeq *seq = (HIgnoreSeq*)env;
  for (size_t i=0; i<seq->len; ++i) {
    if (!seq->parsers[i]->vtable->isValidRegular(seq->parsers[i]->env))
      return false;
  }
  return true;
}

static bool is_isValidCF(void *env) {
  HIgnoreSeq *seq = (HIgnoreSeq*)env;
  for (size_t i=0; i<seq->len; ++i) {
    if (!seq->parsers[i]->vtable->isValidCF(seq->parsers[i]->env))
      return false;
  }
  return true;
}

static bool h_svm_action_ignoreseq(HArena *arena, HSVMContext *ctx, void* env) {
  HIgnoreSeq *seq = (HIgnoreSeq*)env;
  HParsedToken* save = NULL;
  // We can assume that each subitem generated at most one item on the
  // stack.
  assert(seq->len >= 1);
  for (int i = seq->len - 1; i>=0; i--) {
    if (i == (int)seq->which && ctx->stack[ctx->stack_count]->token_type != TT_MARK) 
      save = ctx->stack[ctx->stack_count-1];
    // skip over everything up to and including the mark.
    while (ctx->stack[--ctx->stack_count]->token_type != TT_MARK)
      ;
  }
  ctx->stack[ctx->stack_count++] = save;
  return true;
}

static bool is_ctrvm(HRVMProg *prog, void* env) {
  HIgnoreSeq *seq = (HIgnoreSeq*)env;
  for (size_t i=0; i<seq->len; ++i) {
    h_rvm_insert_insn(prog, RVM_PUSH, 0);
    if (!h_compile_regex(prog, seq->parsers[i]->env))
      return false;
  }
  h_rvm_insert_insn(prog, RVM_ACTION, h_rvm_create_action(prog, h_svm_action_ignoreseq, env));
  return true;
}

static const HParserVtable ignoreseq_vt = {
  .parse = parse_ignoreseq,
  .isValidRegular = is_isValidRegular,
  .isValidCF = is_isValidCF,
  .desugar = desugar_ignoreseq,
  .compile_to_rvm = is_ctrvm,
};


//
// API frontends
//

static HParser* h_leftright__m(HAllocator* mm__, const HParser* p, const HParser* q, size_t which) {
  HIgnoreSeq *seq = h_new(HIgnoreSeq, 1);
  seq->parsers = h_new(const HParser*, 2);
  seq->parsers[0] = p;
  seq->parsers[1] = q;
  seq->len = 2;
  seq->which = which;

  return h_new_parser(mm__, &ignoreseq_vt, seq);
}

HParser* h_left(const HParser* p, const HParser* q) {
  return h_leftright__m(&system_allocator, p, q, 0);
}
HParser* h_left__m(HAllocator* mm__, const HParser* p, const HParser* q) {
  return h_leftright__m(mm__, p, q, 0);
}

HParser* h_right(const HParser* p, const HParser* q) {
  return h_leftright__m(&system_allocator, p, q, 1);
}
HParser* h_right__m(HAllocator* mm__, const HParser* p, const HParser* q) {
  return h_leftright__m(mm__, p, q, 1);
}


HParser* h_middle(const HParser* p, const HParser* x, const HParser* q) {
  return h_middle__m(&system_allocator, p, x, q);
}
HParser* h_middle__m(HAllocator* mm__, const HParser* p, const HParser* x, const HParser* q) {
  HIgnoreSeq *seq = h_new(HIgnoreSeq, 1);
  seq->parsers = h_new(const HParser*, 3);
  seq->parsers[0] = p;
  seq->parsers[1] = x;
  seq->parsers[2] = q;
  seq->len = 3;
  seq->which = 1;

  return h_new_parser(mm__, &ignoreseq_vt, seq);
}
