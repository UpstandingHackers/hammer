/* Internals for Hammer.
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

#ifndef HAMMER_INTERNAL__H
#define HAMMER_INTERNAL__H
#include <glib.h>
#include <err.h>
#include "hammer.h"

#ifdef NDEBUG
#define assert_message(check, message) do { } while(0)
#else
#define assert_message(check, message) do {				\
    if (!(check))							\
      errx(1, "Assertion failed (programmer error): %s", message);	\
  } while(0)
#endif
#define false 0
#define true 1

typedef struct HInputStream_ {
  // This should be considered to be a really big value type.
  const uint8_t *input;
  size_t index;
  size_t length;
  char bit_offset;
  char endianness;
  char overrun;
} HInputStream;

/* The state of the parser.
 *
 * Members:
 *   cache - a hash table describing the state of the parse, including partial HParseResult's. It's a hash table from HParserCacheKey to HParserCacheValue. 
 *   input_stream - the input stream at this state.
 *   arena - the arena that has been allocated for the parse this state is in.
 *   lr_stack - a stack of HLeftRec's, used in Warth's recursion
 *   recursion_heads - table of recursion heads. Keys are HParserCacheKey's with only an HInputStream (parser can be NULL), values are HRecursionHead's.
 *
 */
  
struct HParseState_ {
  GHashTable *cache; 
  HInputStream input_stream;
  HArena * arena;
  GQueue *lr_stack;
  GHashTable *recursion_heads;
};

/* The (location, parser) tuple used to key the cache.
 */

typedef struct HParserCacheKey_ {
  HInputStream input_pos;
  const HParser *parser;
} HParserCacheKey;

/* A value in the cache is either of value Left or Right (this is a 
 * holdover from Scala, which used Either here). Left corresponds to
 * HLeftRec, which is for left recursion; Right corresponds to 
 * HParseResult.
 */

typedef enum HParserCacheValueType_ {
  PC_LEFT,
  PC_RIGHT
} HParserCacheValueType;


/* A recursion head.
 *
 * Members:
 *   head_parser - the parse rule that started this recursion
 *   involved_set - A list of rules (HParser's) involved in the recursion
 *   eval_set - 
 */
typedef struct HRecursionHead_ {
  const HParser *head_parser;
  GSList *involved_set;
  GSList *eval_set;
} HRecursionHead;


/* A left recursion.
 *
 * Members:
 *   seed -
 *   rule -
 *   head -
 */
typedef struct HLeftRec_ {
  HParseResult *seed;
  const HParser *rule;
  HRecursionHead *head;
} HLeftRec;

/* Result and remaining input, for rerunning from a cached position. */
typedef struct HCachedResult_ {
  HParseResult *result;
  HInputStream input_stream;
} HCachedResult;

/* Tagged union for values in the cache: either HLeftRec's (Left) or 
 * HParseResult's (Right).
 */
typedef struct HParserCacheValue_t {
  HParserCacheValueType value_type;
  union {
    HLeftRec *left;
    HCachedResult *right;
  };
} HParserCacheValue;

typedef unsigned int *HCharset;

static inline HCharset new_charset() {
  HCharset cs = g_new0(unsigned int, 256 / sizeof(unsigned int));
  return cs;
}

static inline int charset_isset(HCharset cs, uint8_t pos) {
  return !!(cs[pos / sizeof(*cs)] & (1 << (pos % sizeof(*cs))));
}

static inline void charset_set(HCharset cs, uint8_t pos, int val) {
  cs[pos / sizeof(*cs)] =
    val
    ? cs[pos / sizeof(*cs)] |  (1 << (pos % sizeof(*cs)))
    : cs[pos / sizeof(*cs)] & ~(1 << (pos % sizeof(*cs)));
}

// TODO(thequux): Set symbol visibility for these functions so that they aren't exported.

long long h_read_bits(HInputStream* state, int count, char signed_p);
// need to decide if we want to make this public. 
HParseResult* h_do_parse(const HParser* parser, HParseState *state);
void put_cached(HParseState *ps, const HParser *p, HParseResult *cached);

HCountedArray *h_carray_new_sized(HArena * arena, size_t size);
HCountedArray *h_carray_new(HArena * arena);
void h_carray_append(HCountedArray *array, void* item);


#if 0
#include <malloc.h>
#define arena_malloc(a, s) malloc(s)
#endif

#endif // #ifndef HAMMER_INTERNAL__H
