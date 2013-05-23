/* Parser combinators for binary formats.
 * Copyright (C) 2012  Meredith L. Patterson, Dan "TQ" Hirsch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <assert.h>
#include <ctype.h>
#include <err.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include "hammer.h"
#include "internal.h"
#include "allocator.h"
#include "parsers/parser_internal.h"

static HParserBackendVTable *backends[PB_MAX + 1] = {
  &h__packrat_backend_vtable,
  &h__regex_backend_vtable,
  &h__llk_backend_vtable,
};


/* Helper function, since these lines appear in every parser */

typedef struct {
  const HParser *p1;
  const HParser *p2;
} HTwoParsers;




HParseResult* h_parse(const HParser* parser, const uint8_t* input, size_t length) {
  return h_parse__m(&system_allocator, parser, input, length);
}
HParseResult* h_parse__m(HAllocator* mm__, const HParser* parser, const uint8_t* input, size_t length) {
  // Set up a parse state...
  HInputStream input_stream = {
    .index = 0,
    .bit_offset = 8,
    .overrun = 0,
    .endianness = BIT_BIG_ENDIAN | BYTE_BIG_ENDIAN,
    .length = length,
    .input = input
  };
  
  return backends[parser->backend]->parse(mm__, parser, &input_stream);
}

void h_parse_result_free(HParseResult *result) {
  if(result == NULL) return;
  h_delete_arena(result->arena);
}

bool h_false(void* env) {
  (void)env;
  return false;
}

bool h_true(void* env) {
  (void)env;
  return true;
}

bool h_not_regular(HRVMProg *prog, void *env) {
  (void)env;
  return false;
}

int h_compile(HParser* parser, HParserBackend backend, const void* params) {
  return h_compile__m(&system_allocator, parser, backend, params);
}

int h_compile__m(HAllocator* mm__, HParser* parser, HParserBackend backend, const void* params) {
  backends[parser->backend]->free(parser);
  int ret = backends[backend]->compile(mm__, parser, params);
  if (!ret)
    parser->backend = backend;
  return ret;
}
