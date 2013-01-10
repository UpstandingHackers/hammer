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

#define HAMMER_FN_IMPL_NOARGS(rtype_t, name) \
  rtype_t name(void) {			     \
    return name##__m(system_allocator);	     \
  }					     \
  rtype_t name##__m(HAllocator* mm__)
// Functions with arguments are difficult to forward cleanly. Alas, we will need to forward them manually.

#define h_new(type, count) ((type*)(mm__->alloc(mm__, sizeof(type)*(count))))
#define h_free(addr) (mm__->free(mm__, (addr)))

#define false 0
#define true 1

// This is going to be generally useful.
static inline void h_generic_free(HAllocator *allocator, void* ptr) {
  allocator->free(allocator, ptr);
}

HAllocator system_allocator;


typedef struct HInputStream_ {
  // This should be considered to be a really big value type.
  const uint8_t *input;
  size_t index;
  size_t length;
  char bit_offset;
  char endianness;
  char overrun;
} HInputStream;

typedef struct HSlistNode_ {
  void* elem;
  struct HSlistNode_ *next;
} HSlistNode;

typedef struct HSlist_ {
  HSlistNode *head;
  struct HArena_ *arena;
} HSlist;

typedef unsigned int HHashValue;
typedef HHashValue (*HHashFunc)(const void* key);
typedef bool (*HEqualFunc)(const void* key1, const void* key2);

typedef struct HHashTableEntry_ {
  struct HHashTableEntry_ *next;
  void* key;
  void* value;
  HHashValue hashval;
} HHashTableEntry;

typedef struct HHashTable_ {
  HHashTableEntry *contents;
  HHashFunc hashFunc;
  HEqualFunc equalFunc;
  size_t capacity;
  size_t used;
  HArena *arena;
} HHashTable;

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
  HHashTable *cache; 
  HInputStream input_stream;
  HArena * arena;
  HSlist *lr_stack;
  HHashTable *recursion_heads;
};

typedef struct HParserBackendVTable_ {
  int (*compile)(HAllocator *mm__, const HParser* parser, const void* params);
  HParseResult* (*parse)(HAllocator *mm__, const HParser* parser, HParseState* parse_state);
} HParserBackendVTable;


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
  HSlist *involved_set;
  HSlist *eval_set;
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

// This file provides the logical inverse of bitreader.c
struct HBitWriter_ {
  uint8_t* buf;
  HAllocator *mm__;
  size_t index;
  size_t capacity;
  char bit_offset; // unlike in bit_reader, this is always the number
		   // of used bits in the current byte. i.e., 0 always
		   // means that 8 bits are available for use.
  char flags;
};

// }}}

// Backends {{{
extern HParserBackendVTable h__packrat_backend_vtable;
// }}}

// TODO(thequux): Set symbol visibility for these functions so that they aren't exported.

long long h_read_bits(HInputStream* state, int count, char signed_p);
// need to decide if we want to make this public. 
HParseResult* h_do_parse(const HParser* parser, HParseState *state);
void put_cached(HParseState *ps, const HParser *p, HParseResult *cached);

HCountedArray *h_carray_new_sized(HArena * arena, size_t size);
HCountedArray *h_carray_new(HArena * arena);
void h_carray_append(HCountedArray *array, void* item);

HSlist* h_slist_new(HArena *arena);
HSlist* h_slist_copy(HSlist *slist);
void* h_slist_pop(HSlist *slist);
void h_slist_push(HSlist *slist, void* item);
bool h_slist_find(HSlist *slist, const void* item);
HSlist* h_slist_remove_all(HSlist *slist, const void* item);
void h_slist_free(HSlist *slist);

HHashTable* h_hashtable_new(HArena *arena, HEqualFunc equalFunc, HHashFunc hashFunc);
void* h_hashtable_get(HHashTable* ht, void* key);
void h_hashtable_put(HHashTable* ht, void* key, void* value);
int   h_hashtable_present(HHashTable* ht, void* key);
void  h_hashtable_del(HHashTable* ht, void* key);
void  h_hashtable_free(HHashTable* ht);

#if 0
#include <stdlib.h>
#define h_arena_malloc(a, s) malloc(s)
#endif

#endif // #ifndef HAMMER_INTERNAL__H
