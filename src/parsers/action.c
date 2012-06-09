#include "parser_internal.h"

typedef struct {
  const HParser *p;
  HAction action;
} HParseAction;

static HParseResult* parse_action(void *env, HParseState *state) {
  HParseAction *a = (HParseAction*)env;
  if (a->p && a->action) {
    HParseResult *tmp = h_do_parse(a->p, state);
    //HParsedToken *tok = a->action(h_do_parse(a->p, state));
    if(tmp) {
        const HParsedToken *tok = a->action(tmp);
        return make_result(state, (HParsedToken*)tok);
    } else
      return NULL;
  } else // either the parser's missing or the action's missing
    return NULL;
}

static const HParserVtable action_vt = {
  .parse = parse_action,
};

const HParser* h_action(const HParser* p, const HAction a) { 
  HParser *res = g_new(HParser, 1);
  res->vtable = &action_vt;
  HParseAction *env = g_new(HParseAction, 1);
  env->p = p;
  env->action = a;
  res->env = (void*)env;
  return res;
}
