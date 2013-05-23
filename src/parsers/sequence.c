#include <stdarg.h>
#include <assert.h>
#include "parser_internal.h"

typedef struct {
  size_t len;
  const HParser **p_array;
} HSequence;

static HParseResult* parse_sequence(void *env, HParseState *state) {
  HSequence *s = (HSequence*)env;
  HCountedArray *seq = h_carray_new_sized(state->arena, (s->len > 0) ? s->len : 4);
  for (size_t i=0; i<s->len; ++i) {
    HParseResult *tmp = h_do_parse(s->p_array[i], state);
    // if the interim parse fails, the whole thing fails
    if (NULL == tmp) {
      return NULL;
    } else {
      if (tmp->ast)
	h_carray_append(seq, (void*)tmp->ast);
    }
  }
  HParsedToken *tok = a_new(HParsedToken, 1);
  tok->token_type = TT_SEQUENCE; tok->seq = seq;
  return make_result(state->arena, tok);
}

static bool sequence_isValidRegular(void *env) {
  HSequence *s = (HSequence*)env;
  for (size_t i=0; i<s->len; ++i) {
    if (!s->p_array[i]->vtable->isValidRegular(s->p_array[i]->env))
      return false;
  }
  return true;
}

static bool sequence_isValidCF(void *env) {
  HSequence *s = (HSequence*)env;
  for (size_t i=0; i<s->len; ++i) {
    if (!s->p_array[i]->vtable->isValidCF(s->p_array[i]->env))
      return false;
  }
  return true;
}

static const HParsedToken *reshape_sequence(const HParseResult *p) {
  assert(p->ast);
  assert(p->ast->token_type == TT_SEQUENCE);

  HCountedArray *seq = h_carray_new(p->arena);

  // drop all elements that are NULL
  for(size_t i=0; i<p->ast->seq->used; i++) {
    if(p->ast->seq->elements[i] != NULL)
      h_carray_append(seq, p->ast->seq->elements[i]);
  }

  HParsedToken *res = a_new_(p->arena, HParsedToken, 1);
  res->token_type = TT_SEQUENCE;
  res->seq = seq;
  res->index = p->ast->index;
  res->bit_offset = p->ast->bit_offset;

  return res;
}

static HCFChoice* desugar_sequence(HAllocator *mm__, void *env) {
  HSequence *s = (HSequence*)env;
  HCFSequence *seq = h_new(HCFSequence, 1);
  seq->items = h_new(HCFChoice*, s->len+1);
  for (size_t i=0; i<s->len; ++i) {
    seq->items[i] = h_desugar(mm__, s->p_array[i]);
  }
  seq->items[s->len] = NULL;
  HCFChoice *ret = h_new(HCFChoice, 1);
  ret->type = HCF_CHOICE;
  ret->seq = h_new(HCFSequence*, 2);
  ret->seq[0] = seq;
  ret->seq[1] = NULL;
  ret->action = NULL;
  ret->reshape = reshape_sequence;
  return ret;
}

static bool sequence_ctrvm(HRVMProg *prog, void *env) {
  HSequence *s = (HSequence*)env;
  h_rvm_insert_insn(prog, RVM_PUSH, 0);
  for (size_t i=0; i<s->len; ++i) {
    if (!s->p_array[i]->vtable->compile_to_rvm(prog, s->p_array[i]->env))
      return false;
  }
  h_rvm_insert_insn(prog, RVM_ACTION, h_rvm_create_action(prog, h_svm_action_make_sequence, NULL));
  return true;
}

static const HParserVtable sequence_vt = {
  .parse = parse_sequence,
  .isValidRegular = sequence_isValidRegular,
  .isValidCF = sequence_isValidCF,
  .desugar = desugar_sequence,
  .compile_to_rvm = sequence_ctrvm,
};

HParser* h_sequence(const HParser* p, ...) {
  va_list ap;
  va_start(ap, p);
  HParser* ret = h_sequence__mv(&system_allocator, p,  ap);
  va_end(ap);
  return ret;
}

HParser* h_sequence__m(HAllocator* mm__, const HParser* p, ...) {
  va_list ap;
  va_start(ap, p);
  HParser* ret = h_sequence__mv(mm__, p,  ap);
  va_end(ap);
  return ret;
}

HParser* h_sequence__v(const HParser* p, va_list ap) {
  return h_sequence__mv(&system_allocator, p, ap);
}

HParser* h_sequence__mv(HAllocator* mm__, const HParser *p, va_list ap_) {
  va_list ap;
  size_t len = 0;
  const HParser *arg;
  va_copy(ap, ap_);
  do {
    len++;
    arg = va_arg(ap, const HParser *);
  } while (arg);
  va_end(ap);
  HSequence *s = h_new(HSequence, 1);
  s->p_array = h_new(const HParser *, len);

  va_copy(ap, ap_);
  s->p_array[0] = p;
  for (size_t i = 1; i < len; i++) {
    s->p_array[i] = va_arg(ap, const HParser *);
  } while (arg);
  va_end(ap);

  s->len = len;
  return h_new_parser(mm__, &sequence_vt, s);
}

HParser* h_sequence__a(void *args[]) {
  return h_sequence__ma(&system_allocator, args);
}

HParser* h_sequence__ma(HAllocator* mm__, void *args[]) {
  size_t len = -1; // because do...while
  const HParser *arg;
  
  do {
    arg=((HParser **)args)[++len];
  } while(arg);
  
  HSequence *s = h_new(HSequence, 1);
  s->p_array = h_new(const HParser *, len);

  for (size_t i = 0; i < len; i++) {
    s->p_array[i] = ((HParser **)args)[i];
  }
  
  s->len = len;
  HParser *ret = h_new(HParser, 1);
  ret->vtable = &sequence_vt; ret->env = (void*)s;
  return ret;
}
