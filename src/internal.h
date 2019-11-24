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

/*
 * NOTE: This is an internal header and installed for use by extensions. The
 * API is not guaranteed stable.
*/

#ifndef HAMMER_INTERNAL__H
#define HAMMER_INTERNAL__H
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include "hammer.h"
#include "platform.h"

/* "Internal" in this case means "we're not ready to commit
 * to a public API." Many structures and routines here will be
 * useful in client programs.
 */

#ifdef NDEBUG
#define assert_message(check, message) do { } while(0)
#else
#define assert_message(check, message) do {				\
    if (!(check))							\
      h_platform_errx(1, "Assertion failed (programmer error): %s", message);	\
  } while(0)
#endif

#define HAMMER_FN_IMPL_NOARGS(rtype_t, name) \
  rtype_t name(void) {			     \
    return name##__m(system_allocator);	     \
  }					     \
  rtype_t name##__m(HAllocator* mm__)
// Functions with arguments are difficult to forward cleanly. Alas, we will need to forward them manually.

#define h_new(type, count) ((type*)(h_alloc(mm__, sizeof(type)*(count))))
#define h_free(addr) (mm__->free(mm__, (addr)))

#ifndef __cplusplus
#define false 0
#define true 1
#endif
#ifdef __cplusplus 
extern "C" {
#endif

// This is going to be generally useful.
static inline void h_generic_free(HAllocator *allocator, void* ptr) {
  allocator->free(allocator, ptr);
}

extern HAllocator system_allocator;
typedef struct HCFStack_ HCFStack;


typedef struct HInputStream_ {
  // This should be considered to be a really big value type.
  const uint8_t *input;
  size_t pos;  // position of this chunk in a multi-chunk stream
  size_t index;
  size_t length;
  char bit_offset;
  char margin; // The number of bits on the end that is being read
	       // towards that should be ignored.
  char endianness;
  bool overrun;
  bool last_chunk;
} HInputStream;

typedef struct HSlistNode_ {
  void* elem;
  struct HSlistNode_ *next;
} HSlistNode;

typedef struct HSlist_ {
  HSlistNode *head;
  struct HArena_ *arena;
} HSlist;

// {{{ HSArray

typedef struct HSArrayNode_ {
  size_t elem;
  size_t index;
  void* content;
} HSArrayNode;

typedef struct HSArray_ {
  // Sparse array
  // Element n is valid iff arr->nodes[n].index < arr.used && arr.nodes[arr.nodes[n].index].elem == n
  HSArrayNode *nodes; // content for node at index n is stored at position n.
  size_t capacity;
  size_t used;
  HAllocator *mm__;
} HSArray;

HSArray *h_sarray_new(HAllocator *mm__, size_t size);
void h_sarray_free(HSArray *arr);
static inline bool h_sarray_isset(HSArray *arr, size_t n) {
  assert(n < arr->capacity);
  return (arr->nodes[n].index < arr->used && arr->nodes[arr->nodes[n].index].elem == n);
}
static inline void* h_sarray_get(HSArray *arr, size_t n) {
  assert(n < arr->capacity);
  if (h_sarray_isset(arr, n))
    return arr->nodes[n].content;
  return NULL;
}

static inline void* h_sarray_set(HSArray *arr, size_t n, void* val) {
  assert(n < arr->capacity);
  arr->nodes[n].content = val;
  if (h_sarray_isset(arr, n))
    return val;
  arr->nodes[arr->used].elem = n;
  arr->nodes[n].index = arr->used++;
  return val;
}

static inline void h_sarray_clear(HSArray *arr) {
  arr->used = 0;
}

#define H__APPEND2(a,b) a##b
#define H__APPEND(a,b) H__APPEND2(a,b)
#define H__INTVAR(pfx) H__APPEND(intvar__##pfx##__,__COUNTER__)

#define H_SARRAY_FOREACH_KV_(var,idx,arr,intvar)			\
  for (size_t intvar = 0, idx = (var = (arr)->nodes[(arr)->nodes[intvar].elem].content,(arr)->nodes[intvar].elem); \
       intvar < (arr)->used;						\
       idx = (arr)->nodes[intvar].elem, var = (arr)->nodes[(arr)->nodes[intvar].elem].content, intvar=intvar+1)

#define H_SARRAY_FOREACH_KV(var,index,arr) H_SARRAY_FOREACH_KV_(var,index,arr,H__INTVAR(idx))
#define H_SARRAY_FOREACH_V(var,arr) H_SARRAY_FOREACH_KV_(var,H__INTVAR(elem),arr,H__INTVAR(idx))
#define H_SARRAY_FOREACH_K(index,arr) H_SARRAY_FOREACH_KV_(H__INTVAR(val),index,arr,H__INTVAR(idx))

// }}}

typedef unsigned int *HCharset;

static inline HCharset new_charset(HAllocator* mm__) {
  HCharset cs = h_new(unsigned int, 256 / (sizeof(unsigned int) * 8));
  memset(cs, 0, 32);  // 32 bytes = 256 bits
  return cs;
}

static inline int charset_isset(HCharset cs, uint8_t pos) {
  return !!(cs[pos / (sizeof(*cs)*8)] & (1 << (pos % (sizeof(*cs)*8))));
}

static inline void charset_set(HCharset cs, uint8_t pos, int val) {
  cs[pos / (sizeof(*cs)*8)] =
    val
    ? cs[pos / (sizeof(*cs)*8)] |  (1 << (pos % (sizeof(*cs)*8)))
    : cs[pos / (sizeof(*cs)*8)] & ~(1 << (pos % (sizeof(*cs)*8)));
}

typedef unsigned int HHashValue;
typedef HHashValue (*HHashFunc)(const void* key);
typedef bool (*HEqualFunc)(const void* key1, const void* key2);

typedef struct HHashTableEntry_ {
  struct HHashTableEntry_ *next;
  const void* key;
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
 *   symbol_table - stack of tables of values that have been stashed in the context of this parse.
 *
 */
  
struct HParseState_ {
  HHashTable *cache; 
  HInputStream input_stream;
  HArena * arena;
  HSlist *lr_stack;
  HHashTable *recursion_heads;
  HSlist *symbol_table; // its contents are HHashTables
};

struct HSuspendedParser_ {
  HAllocator *mm__;
  const HParser *parser;
  void *backend_state;
  bool done;

  // input stream state
  size_t pos;
  uint8_t bit_offset;
  uint8_t endianness;
};

typedef struct HParserBackendVTable_ {
  int (*compile)(HAllocator *mm__, HParser* parser, const void* params);
  HParseResult* (*parse)(HAllocator *mm__, const HParser* parser, HInputStream* stream);
  void (*free)(HParser* parser);

  void (*parse_start)(HSuspendedParser *s);
    // parse_start should allocate s->backend_state.
  bool (*parse_chunk)(HSuspendedParser *s, HInputStream *input);
    // if parser is done, return true. otherwise:
    // parse_chunk MUST consume all input, integrating it into s->backend_state.
    // parse_chunk will not be called again after it reports done.
  HParseResult *(*parse_finish)(HSuspendedParser *s);
    // parse_finish must free s->backend_state.
    // parse_finish will not be called before parse_chunk reports done.
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
 *   seed - the HResult yielded by rule
 *   rule - the HParser that produces seed
 *   head - the 
 */
typedef struct HLeftRec_ {
  HParseResult *seed;
  const HParser *rule;
  HRecursionHead *head;
} HLeftRec;

/* Tagged union for values in the cache: either HLeftRec's (Left) or 
 * HParseResult's (Right).
 * Includes the position (input_stream) to advance to after using this value.
 */
typedef struct HParserCacheValue_t {
  HParserCacheValueType value_type;
  union {
    HLeftRec *left;
    HParseResult *right;
  };
  HInputStream input_stream;
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
  char error;
};

// }}}


// Backends {{{
extern HParserBackendVTable h__packrat_backend_vtable;
extern HParserBackendVTable h__llk_backend_vtable;
extern HParserBackendVTable h__lalr_backend_vtable;
extern HParserBackendVTable h__glr_backend_vtable;
// }}}

// TODO(thequux): Set symbol visibility for these functions so that they aren't exported.

int64_t h_read_bits(HInputStream* state, int count, char signed_p);
void h_skip_bits(HInputStream* state, size_t count);
void h_seek_bits(HInputStream* state, size_t pos);
static inline size_t h_input_stream_pos(HInputStream* state) {
  assert(state->index < SIZE_MAX / 8);
  return state->index * 8 + state->bit_offset + state->margin;
}
static inline size_t h_input_stream_length(HInputStream *state) {
  assert(state->length <= SIZE_MAX / 8);
  return state->length * 8;
}
// need to decide if we want to make this public. 
HParseResult* h_do_parse(const HParser* parser, HParseState *state);
void put_cached(HParseState *ps, const HParser *p, HParseResult *cached);

static inline
HParser *h_new_parser(HAllocator *mm__, const HParserVtable *vt, void *env) {
  HParser *p = h_new(HParser, 1);
  memset(p, 0, sizeof(HParser));
  p->vtable = vt;
  p->env = env;
  return p;
}

HCFChoice *h_desugar(HAllocator *mm__, HCFStack *stk__, const HParser *parser);

HCountedArray *h_carray_new_sized(HArena * arena, size_t size);
HCountedArray *h_carray_new(HArena * arena);
void h_carray_append(HCountedArray *array, void* item);

HSlist* h_slist_new(HArena *arena);
HSlist* h_slist_copy(HSlist *slist);
void* h_slist_pop(HSlist *slist);
void* h_slist_drop(HSlist *slist);
static inline void* h_slist_top(HSlist *sl) { return sl->head->elem; }
void h_slist_push(HSlist *slist, void* item);
bool h_slist_find(HSlist *slist, const void* item);
HSlist* h_slist_remove_all(HSlist *slist, const void* item);
void h_slist_free(HSlist *slist);
static inline bool h_slist_empty(const HSlist *sl) { return (sl->head == NULL); }

HHashTable* h_hashtable_new(HArena *arena, HEqualFunc equalFunc, HHashFunc hashFunc);
void* h_hashtable_get(const HHashTable* ht, const void* key);
void  h_hashtable_put(HHashTable* ht, const void* key, void* value);
void  h_hashtable_update(HHashTable* dst, const HHashTable *src);
void  h_hashtable_merge(void *(*combine)(void *v1, const void *v2),
                        HHashTable *dst, const HHashTable *src);
int   h_hashtable_present(const HHashTable* ht, const void* key);
void  h_hashtable_del(HHashTable* ht, const void* key);
void  h_hashtable_free(HHashTable* ht);
static inline bool h_hashtable_empty(const HHashTable* ht) { return (ht->used == 0); }

typedef HHashTable HHashSet;
#define h_hashset_new(a,eq,hash) h_hashtable_new(a,eq,hash)
#define h_hashset_put(ht,el)     h_hashtable_put(ht, el, NULL)
#define h_hashset_put_all(a,b)   h_hashtable_update(a, b)
#define h_hashset_present(ht,el) h_hashtable_present(ht,el)
#define h_hashset_empty(ht)      h_hashtable_empty(ht)
#define h_hashset_del(ht,el)     h_hashtable_del(ht,el)
#define h_hashset_free(ht)       h_hashtable_free(ht)
bool h_hashset_equal(const HHashSet *a, const HHashSet *b);

bool h_eq_ptr(const void *p, const void *q);
HHashValue h_hash_ptr(const void *p);
uint32_t h_djbhash(const uint8_t *buf, size_t len);

void h_symbol_put(HParseState *state, const char* key, void *value);
void* h_symbol_get(HParseState *state, const char* key);

typedef struct HCFSequence_ HCFSequence;

struct HCFChoice_ {
  enum HCFChoiceType {
    HCF_END,
    HCF_CHOICE,
    HCF_CHARSET,
    HCF_CHAR
  } type;
  union {
    HCharset charset;
    HCFSequence** seq;
    uint8_t chr;
  };
  HAction reshape;  // take CFG parse tree to HParsedToken of expected form.
                    // to execute before action and pred are applied.
  HAction action;
  HPredicate pred;
  void* user_data;
};

struct HCFSequence_ {
  HCFChoice **items; // last one is NULL
};

struct HParserVtable_ {
  HParseResult* (*parse)(void *env, HParseState *state);
  bool (*isValidRegular)(void *env);
  bool (*isValidCF)(void *env);
  bool (*compile_to_rvm)(HRVMProg *prog, void* env); // FIXME: forgot what the bool return value was supposed to mean.
  void (*desugar)(HAllocator *mm__, HCFStack *stk__, void *env);
  bool higher; // false if primitive
};

// {{{ Token type registry internal

typedef struct HTTEntry_ {
  const char* name;
  HTokenType value;
  void (*unamb_sub)(const HParsedToken *tok, struct result_buf *buf);
} HTTEntry;

const HTTEntry* h_get_token_type_entry(HTokenType token_type);

// }}}

bool h_false(void*);
bool h_true(void*);
bool h_not_regular(HRVMProg*, void*);

#if 0
#include <stdlib.h>
#define h_arena_malloc(a, s) malloc(s)
#endif
#ifdef __cplusplus
} // extern "C"
#endif
#endif // #ifndef HAMMER_INTERNAL__H
