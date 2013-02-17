#ifndef HAMMER_DNS_COMMON__H
#define HAMMER_DNS_COMMON__H

#include "../src/hammer.h"
#include "../src/glue.h"

const HParser* init_domain();
const HParser* init_character_string();

const HParsedToken* act_index0(const HParseResult *p);

#endif
