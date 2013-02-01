#include "../src/hammer.h"
#include "dns_common.h"
#include "dns.h"

#define false 0
#define true 1

H_ACT_APPLY(act_index0, h_act_index, 0)

/**
 * A label can't be more than 63 characters.
 */
bool validate_label(HParseResult *p) {
  if (TT_SEQUENCE != p->ast->token_type)
    return false;
  return (64 > p->ast->seq->used);
}

#define act_label h_act_flatten

const HParsedToken* act_domain(const HParseResult *p) {
  const HParsedToken *ret = NULL;
  char *arr = NULL;

  switch(p->ast->token_type) {
  case TT_UINT:
    arr = " ";
    break;
  case TT_SEQUENCE:
    // Sequence of subdomains separated by "."
    // Each subdomain is a label, which can be no more than 63 chars.
    arr = h_arena_malloc(p->arena, 64*p->ast->seq->used);
    size_t count = 0;
    for (size_t i=0; i<p->ast->seq->used; ++i) {
      HParsedToken *tmp = p->ast->seq->elements[i];
      for (size_t j=0; j<tmp->seq->used; ++j) {
	arr[count] = tmp->seq->elements[i]->uint;
	++count;
      }
      arr[count] = '.';
      ++count;
    }
    arr[count-1] = '\x00';
    break;
  default:
    arr = NULL;
    ret = NULL;
  }

  if(arr) {
    dns_domain_t *val = H_ALLOC(dns_domain_t);  // dns_domain_t is char*
    *val = arr;
    ret = H_MAKE(dns_domain_t, val);
  }

  return ret;
}

const HParser* init_domain() {
  static const HParser *ret = NULL;
  if (ret)
    return ret;

  H_RULE  (letter,    h_choice(h_ch_range('a','z'), h_ch_range('A','Z'), NULL));
  H_RULE  (let_dig,   h_choice(letter, h_ch_range('0','9'), NULL));
  H_RULE  (ldh_str,   h_many1(h_choice(let_dig, h_ch('-'), NULL)));
  H_VARULE(label,     h_sequence(letter,
				 h_optional(h_sequence(h_optional(ldh_str),
						       let_dig,
						       NULL)),
				 NULL));
  H_RULE  (subdomain, h_sepBy1(label, h_ch('.')));
  H_ARULE (domain,    h_choice(subdomain, h_ch(' '), NULL));

  ret = domain;
  return ret;
}

const HParser* init_character_string() {
  static const HParser *cstr = NULL;
  if (cstr)
    return cstr;

  cstr = h_length_value(h_uint8(), h_uint8());

  return cstr;
}
