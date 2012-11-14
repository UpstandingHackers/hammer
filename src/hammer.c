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

static uint32_t djbhash(const uint8_t *buf, size_t len) {
  uint32_t hash = 5381;
  while (len--) {
    hash = hash * 33 + *buf++;
  }
  return hash;
}

/* Helper function, since these lines appear in every parser */

typedef struct {
  const HParser *p1;
  const HParser *p2;
} HTwoParsers;


static uint32_t cache_key_hash(const void* key) {
  return djbhash(key, sizeof(HParserCacheKey));
}
static bool cache_key_equal(const void* key1, const void* key2) {
  return memcmp(key1, key2, sizeof(HParserCacheKey)) == 0;
}


HParseResult* h_parse(const HParser* parser, const uint8_t* input, size_t length) {
  return h_parse__m(&system_allocator, parser, input, length);
}
HParseResult* h_parse__m(HAllocator* mm__, const HParser* parser, const uint8_t* input, size_t length) { 
  // Set up a parse state...
  HArena * arena = h_new_arena(mm__, 0);
  HParseState *parse_state = a_new_(arena, HParseState, 1);
  parse_state->cache = h_hashtable_new(arena, cache_key_equal, // key_equal_func
					      cache_key_hash); // hash_func
  parse_state->input_stream.input = input;
  parse_state->input_stream.index = 0;
  parse_state->input_stream.bit_offset = 8; // bit big endian
  parse_state->input_stream.overrun = 0;
  parse_state->input_stream.endianness = BIT_BIG_ENDIAN | BYTE_BIG_ENDIAN;
  parse_state->input_stream.length = length;
  parse_state->lr_stack = h_slist_new(arena);
  parse_state->recursion_heads = h_hashtable_new(arena, cache_key_equal,
						  cache_key_hash);
  parse_state->arena = arena;
  HParseResult *res = h_do_parse(parser, parse_state);
  h_slist_free(parse_state->lr_stack);
  h_hashtable_free(parse_state->recursion_heads);
  // tear down the parse state
  h_hashtable_free(parse_state->cache);
  if (!res)
    h_delete_arena(parse_state->arena);

  return res;
}

void h_parse_result_free(HParseResult *result) {
  h_delete_arena(result->arena);
}


