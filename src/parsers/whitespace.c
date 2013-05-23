#include <ctype.h>
#include <assert.h>
#include "parser_internal.h"

static HParseResult* parse_whitespace(void* env, HParseState *state) {
  char c;
  HInputStream bak;
  do {
    bak = state->input_stream;
    c = h_read_bits(&state->input_stream, 8, false);
    if (state->input_stream.overrun)
      break;
  } while (isspace(c));
  state->input_stream = bak;
  return h_do_parse((HParser*)env, state);
}

static const char SPACE_CHRS[6] = {' ', '\f', '\n', '\r', '\t', '\v'};

static HCFChoice* desugar_whitespace(HAllocator *mm__, void *env) {
  HCFChoice *ws = h_new(HCFChoice, 1);
  ws->type = HCF_CHOICE;
  ws->seq = h_new(HCFSequence*, 3);
  HCFSequence *nonempty = h_new(HCFSequence, 1);
  nonempty->items = h_new(HCFChoice*, 3);
  nonempty->items[0] = h_new(HCFChoice, 1);
  nonempty->items[0]->type = HCF_CHARSET;
  nonempty->items[0]->charset = new_charset(mm__);
  for(size_t i=0; i<sizeof(SPACE_CHRS); i++)
    charset_set(nonempty->items[0]->charset, SPACE_CHRS[i], 1);
  nonempty->items[1] = ws; // yay circular pointer!
  nonempty->items[2] = NULL;
  ws->seq[0] = nonempty;
  HCFSequence *empty = h_new(HCFSequence, 1);
  empty->items = h_new(HCFChoice*, 1);
  empty->items[0] = NULL;
  ws->seq[1] = empty;
  ws->seq[2] = NULL;

  HCFChoice *ret = h_new(HCFChoice, 1);
  ret->type = HCF_CHOICE;
  ret->seq = h_new(HCFSequence*, 2);
  ret->seq[0] = h_new(HCFSequence, 1);
  ret->seq[0]->items = h_new(HCFChoice*, 3);
  ret->seq[0]->items[0] = ws;
  ret->seq[0]->items[1] = h_desugar(mm__, (HParser *)env);
  ret->seq[0]->items[2] = NULL;
  ret->seq[1] = NULL;

  ret->reshape = h_act_last;
  
  return ret;
}

static bool ws_isValidRegular(void *env) {
  HParser *p = (HParser*)env;
  return p->vtable->isValidRegular(p->env);
}

static bool ws_isValidCF(void *env) {
  HParser *p = (HParser*)env;
  return p->vtable->isValidCF(p->env);
}

static bool ws_ctrvm(HRVMProg *prog, void *env) {
  HParser *p = (HParser*)env;
  uint16_t start = h_rvm_get_ip(prog);
  uint16_t next;

  for (int i = 0; i < 6; i++) {
    next = h_rvm_insert_insn(prog, RVM_FORK, 0);
    h_rvm_insert_insn(prog, RVM_MATCH, (SPACE_CHRS[i] << 8) | (SPACE_CHRS[i]));
    h_rvm_insert_insn(prog, RVM_GOTO, start);
    h_rvm_patch_arg(prog, next, h_rvm_get_ip(prog));
  }
  return h_compile_regex(prog, p->env);
}

static const HParserVtable whitespace_vt = {
  .parse = parse_whitespace,
  .isValidRegular = ws_isValidRegular,
  .isValidCF = ws_isValidCF,
  .desugar = desugar_whitespace,
  .compile_to_rvm = ws_ctrvm,
};

HParser* h_whitespace(const HParser* p) {
  return h_whitespace__m(&system_allocator, p);
}
HParser* h_whitespace__m(HAllocator* mm__, const HParser* p) {
  return h_new_parser(mm__, &whitespace_vt, (void *)p);
}
