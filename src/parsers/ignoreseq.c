#include "parser_internal.h"


//
// general case: parse sequence, pick one result
//

typedef struct {
  const HParser **parsers;
  size_t count;         // how many parsers in 'ps'
  size_t which;         // whose result to return
} HIgnoreSeq;

static HParseResult* parse_ignoreseq(void* env, HParseState *state) {
  const HIgnoreSeq *seq = (HIgnoreSeq*)env;
  HParseResult *res = NULL;

  for (size_t i=0; i < seq->count; ++i) {
    HParseResult *tmp = h_do_parse(seq->parsers[i], state);
    if (!tmp)
      return NULL;
    else if (i == seq->which)
      res = tmp;
  }

  return res;
}

static const HParserVtable ignoreseq_vt = {
  .parse = parse_ignoreseq,
};


//
// API frontends
//

static const HParser* h_leftright(const HParser* p, const HParser* q, size_t which) {
  HIgnoreSeq *seq = g_new(HIgnoreSeq, 1);
  seq->parsers = g_new(const HParser*, 2);
  seq->parsers[0] = p;
  seq->parsers[1] = q;
  seq->count = 2;
  seq->which = which;

  HParser *ret = g_new(HParser, 1);
  ret->vtable = &ignoreseq_vt;
  ret->env = (void*)seq;
  return ret;
}

const HParser* h_left(const HParser* p, const HParser* q) {
  return h_leftright(p, q, 0);
}

const HParser* h_right(const HParser* p, const HParser* q) {
  return h_leftright(p, q, 1);
}

const HParser* h_middle(const HParser* p, const HParser* x, const HParser* q) {
  HIgnoreSeq *seq = g_new(HIgnoreSeq, 1);
  seq->parsers = g_new(const HParser*, 3);
  seq->parsers[0] = p;
  seq->parsers[1] = x;
  seq->parsers[2] = q;
  seq->count = 3;
  seq->which = 1;

  HParser *ret = g_new(HParser, 1);
  ret->vtable = &ignoreseq_vt;
  ret->env = (void*)seq;
  return ret;
}
