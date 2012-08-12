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
#include <error.h>
#include <limits.h>
#include <stdarg.h>
#include <string.h>
#include "hammer.h"
#include "internal.h"
#include "allocator.h"
#include "parsers/parser_internal.h"

static guint djbhash(const uint8_t *buf, size_t len) {
  guint hash = 5381;
  while (len--) {
    hash = hash * 33 + *buf++;
  }
  return hash;
}

// short-hand for constructing HCachedResult's
static HCachedResult *cached_result(const HParseState *state, HParseResult *result) {
  HCachedResult *ret = a_new(HCachedResult, 1);
  ret->result = result;
  ret->input_stream = state->input_stream;
  return ret;
}

// Really library-internal tool to perform an uncached parse, and handle any common error-handling.
static inline HParseResult* perform_lowlevel_parse(HParseState *state, const HParser *parser) {
  // TODO(thequux): these nested conditions are ugly. Factor this appropriately, so that it is clear which codes is executed when.
  HParseResult *tmp_res;
  if (parser) {
    HInputStream bak = state->input_stream;
    tmp_res = parser->vtable->parse(parser->env, state);
    if (tmp_res) {
      tmp_res->arena = state->arena;
      if (!state->input_stream.overrun) {
	tmp_res->bit_length = ((state->input_stream.index - bak.index) << 3);
	if (state->input_stream.endianness & BIT_BIG_ENDIAN)
	  tmp_res->bit_length += state->input_stream.bit_offset - bak.bit_offset;
	else
	  tmp_res->bit_length += bak.bit_offset - state->input_stream.bit_offset;
      } else
	tmp_res->bit_length = 0;
    }
  } else
    tmp_res = NULL;
  if (state->input_stream.overrun)
    return NULL; // overrun is always failure.
#ifdef CONSISTENCY_CHECK
  if (!tmp_res) {
    state->input_stream = INVALID;
    state->input_stream.input = key->input_pos.input;
  }
#endif
  return tmp_res;
}

HParserCacheValue* recall(HParserCacheKey *k, HParseState *state) {
  HParserCacheValue *cached = g_hash_table_lookup(state->cache, k);
  HRecursionHead *head = g_hash_table_lookup(state->recursion_heads, k);
  if (!head) { // No heads found
    return cached;
  } else { // Some heads found
    if (!cached && head->head_parser != k->parser && !g_slist_find(head->involved_set, k->parser)) {
      // Nothing in the cache, and the key parser is not involved
      HParseResult *tmp = a_new(HParseResult, 1);
      tmp->ast = NULL; tmp->arena = state->arena;
      HParserCacheValue *ret = a_new(HParserCacheValue, 1);
      ret->value_type = PC_RIGHT; ret->right = cached_result(state, tmp);
      return ret;
    }
    if (g_slist_find(head->eval_set, k->parser)) {
      // Something is in the cache, and the key parser is in the eval set. Remove the key parser from the eval set of the head. 
      head->eval_set = g_slist_remove_all(head->eval_set, k->parser);
      HParseResult *tmp_res = perform_lowlevel_parse(state, k->parser);
      // we know that cached has an entry here, modify it
      if (!cached)
	cached = a_new(HParserCacheValue, 1);
      cached->value_type = PC_RIGHT;
      cached->right = cached_result(state, tmp_res);
    }
    return cached;
  }
}

/* Setting up the left recursion. We have the LR for the rule head;
 * we modify the involved_sets of all LRs in the stack, until we 
 * see the current parser again.
 */

void setupLR(const HParser *p, HParseState *state, HLeftRec *rec_detect) {
  if (!rec_detect->head) {
    HRecursionHead *some = a_new(HRecursionHead, 1);
    some->head_parser = p; some->involved_set = NULL; some->eval_set = NULL;
    rec_detect->head = some;
  }
  size_t i = 0;
  HLeftRec *lr = g_queue_peek_nth(state->lr_stack, i);
  while (lr && lr->rule != p) {
    lr->head = rec_detect->head;
    lr->head->involved_set = g_slist_prepend(lr->head->involved_set, (gpointer)lr->rule);
  }
}

/* If recall() returns NULL, we need to store a dummy failure in the cache and compute the
 * future parse. 
 */

HParseResult* grow(HParserCacheKey *k, HParseState *state, HRecursionHead *head) {
  // Store the head into the recursion_heads
  g_hash_table_replace(state->recursion_heads, k, head);
  HParserCacheValue *old_cached = g_hash_table_lookup(state->cache, k);
  if (!old_cached || PC_LEFT == old_cached->value_type)
    errx(1, "impossible match");
  HParseResult *old_res = old_cached->right->result;
  
  // reset the eval_set of the head of the recursion at each beginning of growth
  head->eval_set = head->involved_set;
  HParseResult *tmp_res = perform_lowlevel_parse(state, k->parser);

  if (tmp_res) {
    if ((old_res->ast->index < tmp_res->ast->index) || 
	(old_res->ast->index == tmp_res->ast->index && old_res->ast->bit_offset < tmp_res->ast->bit_offset)) {
      HParserCacheValue *v = a_new(HParserCacheValue, 1);
      v->value_type = PC_RIGHT; v->right = cached_result(state, tmp_res);
      g_hash_table_replace(state->cache, k, v);
      return grow(k, state, head);
    } else {
      // we're done with growing, we can remove data from the recursion head
      g_hash_table_remove(state->recursion_heads, k);
      HParserCacheValue *cached = g_hash_table_lookup(state->cache, k);
      if (cached && PC_RIGHT == cached->value_type) {
	return cached->right->result;
      } else {
	errx(1, "impossible match");
      }
    }
  } else {
    g_hash_table_remove(state->recursion_heads, k);
    return old_res;
  }
}

HParseResult* lr_answer(HParserCacheKey *k, HParseState *state, HLeftRec *growable) {
  if (growable->head) {
    if (growable->head->head_parser != k->parser) {
      // not the head rule, so not growing
      return growable->seed;
    }
    else {
      // update cache
      HParserCacheValue *v = a_new(HParserCacheValue, 1);
      v->value_type = PC_RIGHT; v->right = cached_result(state, growable->seed);
      g_hash_table_replace(state->cache, k, v);
      if (!growable->seed)
	return NULL;
      else
	return grow(k, state, growable->head);
    }
  } else {
    errx(1, "lrAnswer with no head");
  }
}

/* Warth's recursion. Hi Alessandro! */
HParseResult* h_do_parse(const HParser* parser, HParseState *state) {
  HParserCacheKey *key = a_new(HParserCacheKey, 1);
  key->input_pos = state->input_stream; key->parser = parser;
  HParserCacheValue *m = recall(key, state);
  // check to see if there is already a result for this object...
  if (!m) {
    // It doesn't exist, so create a dummy result to cache
    HLeftRec *base = a_new(HLeftRec, 1);
    base->seed = NULL; base->rule = parser; base->head = NULL;
    g_queue_push_head(state->lr_stack, base);
    // cache it
    HParserCacheValue *dummy = a_new(HParserCacheValue, 1);
    dummy->value_type = PC_LEFT; dummy->left = base;
    g_hash_table_replace(state->cache, key, dummy);
    // parse the input
    HParseResult *tmp_res = perform_lowlevel_parse(state, parser);
    // the base variable has passed equality tests with the cache
    g_queue_pop_head(state->lr_stack);
    // setupLR, used below, mutates the LR to have a head if appropriate, so we check to see if we have one
    if (NULL == base->head) {
      HParserCacheValue *right = a_new(HParserCacheValue, 1);
      right->value_type = PC_RIGHT; right->right = cached_result(state, tmp_res);
      g_hash_table_replace(state->cache, key, right);
      return tmp_res;
    } else {
      base->seed = tmp_res;
      HParseResult *res = lr_answer(key, state, base);
      return res;
    }
  } else {
    // it exists!
    if (PC_LEFT == m->value_type) {
      setupLR(parser, state, m->left);
      return m->left->seed; // BUG: this might not be correct
    } else {
      state->input_stream = m->right->input_stream;
      return m->right->result;
    }
  }
}

/* Helper function, since these lines appear in every parser */

typedef struct {
  const HParser *p1;
  const HParser *p2;
} HTwoParsers;


static guint cache_key_hash(gconstpointer key) {
  return djbhash(key, sizeof(HParserCacheKey));
}
static gboolean cache_key_equal(gconstpointer key1, gconstpointer key2) {
  return memcmp(key1, key2, sizeof(HParserCacheKey)) == 0;
}


HParseResult* h_parse(const HParser* parser, const uint8_t* input, size_t length) { 
  // Set up a parse state...
  HArena * arena = h_new_arena(0);
  HParseState *parse_state = a_new_(arena, HParseState, 1);
  parse_state->cache = g_hash_table_new(cache_key_hash, // hash_func
					cache_key_equal);// key_equal_func
  parse_state->input_stream.input = input;
  parse_state->input_stream.index = 0;
  parse_state->input_stream.bit_offset = 8; // bit big endian
  parse_state->input_stream.overrun = 0;
  parse_state->input_stream.endianness = BIT_BIG_ENDIAN | BYTE_BIG_ENDIAN;
  parse_state->input_stream.length = length;
  parse_state->lr_stack = g_queue_new();
  parse_state->recursion_heads = g_hash_table_new(cache_key_hash,
						  cache_key_equal);
  parse_state->arena = arena;
  HParseResult *res = h_do_parse(parser, parse_state);
  g_queue_free(parse_state->lr_stack);
  g_hash_table_destroy(parse_state->recursion_heads);
  // tear down the parse state
  g_hash_table_destroy(parse_state->cache);
  if (!res)
    h_delete_arena(parse_state->arena);

  return res;
}

void h_parse_result_free(HParseResult *result) {
  h_delete_arena(result->arena);
}

#ifdef INCLUDE_TESTS

#include "test_suite.h"
static void test_token(void) {
  const HParser *token_ = h_token((const uint8_t*)"95\xa2", 3);

  g_check_parse_ok(token_, "95\xa2", 3, "<39.35.a2>");
  g_check_parse_failed(token_, "95", 2);
}

static void test_ch(void) {
  const HParser *ch_ = h_ch(0xa2);

  g_check_parse_ok(ch_, "\xa2", 1, "u0xa2");
  g_check_parse_failed(ch_, "\xa3", 1);
}

static void test_ch_range(void) {
  const HParser *range_ = h_ch_range('a', 'c');

  g_check_parse_ok(range_, "b", 1, "u0x62");
  g_check_parse_failed(range_, "d", 1);
}

//@MARK_START
static void test_int64(void) {
  const HParser *int64_ = h_int64();

  g_check_parse_ok(int64_, "\xff\xff\xff\xfe\x00\x00\x00\x00", 8, "s-0x200000000");
  g_check_parse_failed(int64_, "\xff\xff\xff\xfe\x00\x00\x00", 7);
}

static void test_int32(void) {
  const HParser *int32_ = h_int32();

  g_check_parse_ok(int32_, "\xff\xfe\x00\x00", 4, "s-0x20000");
  g_check_parse_failed(int32_, "\xff\xfe\x00", 3);
}

static void test_int16(void) {
  const HParser *int16_ = h_int16();

  g_check_parse_ok(int16_, "\xfe\x00", 2, "s-0x200");
  g_check_parse_failed(int16_, "\xfe", 1);
}

static void test_int8(void) {
  const HParser *int8_ = h_int8();

  g_check_parse_ok(int8_, "\x88", 1, "s-0x78");
  g_check_parse_failed(int8_, "", 0);
}

static void test_uint64(void) {
  const HParser *uint64_ = h_uint64();

  g_check_parse_ok(uint64_, "\x00\x00\x00\x02\x00\x00\x00\x00", 8, "u0x200000000");
  g_check_parse_failed(uint64_, "\x00\x00\x00\x02\x00\x00\x00", 7);
}

static void test_uint32(void) {
  const HParser *uint32_ = h_uint32();

  g_check_parse_ok(uint32_, "\x00\x02\x00\x00", 4, "u0x20000");
  g_check_parse_failed(uint32_, "\x00\x02\x00", 3);
}

static void test_uint16(void) {
  const HParser *uint16_ = h_uint16();

  g_check_parse_ok(uint16_, "\x02\x00", 2, "u0x200");
  g_check_parse_failed(uint16_, "\x02", 1);
}

static void test_uint8(void) {
  const HParser *uint8_ = h_uint8();

  g_check_parse_ok(uint8_, "\x78", 1, "u0x78");
  g_check_parse_failed(uint8_, "", 0);
}
//@MARK_END

static void test_int_range(void) {
  const HParser *int_range_ = h_int_range(h_uint8(), 3, 10);
  
  g_check_parse_ok(int_range_, "\x05", 1, "u0x5");
  g_check_parse_failed(int_range_, "\xb", 1);
}

#if 0
static void test_float64(void) {
  const HParser *float64_ = h_float64();

  g_check_parse_ok(float64_, "\x3f\xf0\x00\x00\x00\x00\x00\x00", 8, 1.0);
  g_check_parse_failed(float64_, "\x3f\xf0\x00\x00\x00\x00\x00", 7);
}

static void test_float32(void) {
  const HParser *float32_ = h_float32();

  g_check_parse_ok(float32_, "\x3f\x80\x00\x00", 4, 1.0);
  g_check_parse_failed(float32_, "\x3f\x80\x00");
}
#endif


static void test_whitespace(void) {
  const HParser *whitespace_ = h_whitespace(h_ch('a'));

  g_check_parse_ok(whitespace_, "a", 1, "u0x61");
  g_check_parse_ok(whitespace_, " a", 2, "u0x61");
  g_check_parse_ok(whitespace_, "  a", 3, "u0x61");
  g_check_parse_ok(whitespace_, "\ta", 2, "u0x61");
  g_check_parse_failed(whitespace_, "_a", 2);
}

static void test_left(void) {
  const HParser *left_ = h_left(h_ch('a'), h_ch(' '));

  g_check_parse_ok(left_, "a ", 2, "u0x61");
  g_check_parse_failed(left_, "a", 1);
  g_check_parse_failed(left_, " ", 1);
  g_check_parse_failed(left_, "ab", 2);
}

static void test_right(void) {
  const HParser *right_ = h_right(h_ch(' '), h_ch('a'));

  g_check_parse_ok(right_, " a", 2, "u0x61");
  g_check_parse_failed(right_, "a", 1);
  g_check_parse_failed(right_, " ", 1);
  g_check_parse_failed(right_, "ba", 2);
}

static void test_middle(void) {
  const HParser *middle_ = h_middle(h_ch(' '), h_ch('a'), h_ch(' '));

  g_check_parse_ok(middle_, " a ", 3, "u0x61");
  g_check_parse_failed(middle_, "a", 1);
  g_check_parse_failed(middle_, " ", 1);
  g_check_parse_failed(middle_, " a", 2);
  g_check_parse_failed(middle_, "a ", 2);
  g_check_parse_failed(middle_, " b ", 3);
  g_check_parse_failed(middle_, "ba ", 3);
  g_check_parse_failed(middle_, " ab", 3);
}

#include <ctype.h>

const HParsedToken* upcase(const HParseResult *p) {
  switch(p->ast->token_type) {
  case TT_SEQUENCE:
    {
      HParsedToken *ret = a_new_(p->arena, HParsedToken, 1);
      HCountedArray *seq = h_carray_new_sized(p->arena, p->ast->seq->used);
      ret->token_type = TT_SEQUENCE;
      for (size_t i=0; i<p->ast->seq->used; ++i) {
	if (TT_UINT == ((HParsedToken*)p->ast->seq->elements[i])->token_type) {
	  HParsedToken *tmp = a_new_(p->arena, HParsedToken, 1);
	  tmp->token_type = TT_UINT;
	  tmp->uint = toupper(((HParsedToken*)p->ast->seq->elements[i])->uint);
	  h_carray_append(seq, tmp);
	} else {
	  h_carray_append(seq, p->ast->seq->elements[i]);
	}
      }
      ret->seq = seq;
      return (const HParsedToken*)ret;
    }
  case TT_UINT:
    {
      HParsedToken *ret = a_new_(p->arena, HParsedToken, 1);
      ret->token_type = TT_UINT;
      ret->uint = toupper(p->ast->uint);
      return (const HParsedToken*)ret;
    }
  default:
    return p->ast;
  }
}

static void test_action(void) {
  const HParser *action_ = h_action(h_sequence(h_choice(h_ch('a'), 
							h_ch('A'), 
							NULL), 
					       h_choice(h_ch('b'), 
							h_ch('B'), 
						      NULL), 
					       NULL), 
				    upcase);
  
  g_check_parse_ok(action_, "ab", 2, "(u0x41 u0x42)");
  g_check_parse_ok(action_, "AB", 2, "(u0x41 u0x42)");
  g_check_parse_failed(action_, "XX", 2);
}

static void test_in(void) {
  uint8_t options[3] = { 'a', 'b', 'c' };
  const HParser *in_ = h_in(options, 3);
  g_check_parse_ok(in_, "b", 1, "u0x62");
  g_check_parse_failed(in_, "d", 1);

}

static void test_not_in(void) {
  uint8_t options[3] = { 'a', 'b', 'c' };
  const HParser *not_in_ = h_not_in(options, 3);
  g_check_parse_ok(not_in_, "d", 1, "u0x64");
  g_check_parse_failed(not_in_, "a", 1);

}

static void test_end_p(void) {
  const HParser *end_p_ = h_sequence(h_ch('a'), h_end_p(), NULL);
  g_check_parse_ok(end_p_, "a", 1, "(u0x61)");
  g_check_parse_failed(end_p_, "aa", 2);
}

static void test_nothing_p(void) {
  const HParser *nothing_p_ = h_nothing_p();
  g_check_parse_failed(nothing_p_, "a", 1);
}

static void test_sequence(void) {
  const HParser *sequence_1 = h_sequence(h_ch('a'), h_ch('b'), NULL);
  const HParser *sequence_2 = h_sequence(h_ch('a'), h_whitespace(h_ch('b')), NULL);

  g_check_parse_ok(sequence_1, "ab", 2, "(u0x61 u0x62)");
  g_check_parse_failed(sequence_1, "a", 1);
  g_check_parse_failed(sequence_1, "b", 1);
  g_check_parse_ok(sequence_2, "ab", 2, "(u0x61 u0x62)");
  g_check_parse_ok(sequence_2, "a b", 3, "(u0x61 u0x62)");
  g_check_parse_ok(sequence_2, "a  b", 4, "(u0x61 u0x62)");  
}

static void test_choice(void) {
  const HParser *choice_ = h_choice(h_ch('a'), h_ch('b'), NULL);

  g_check_parse_ok(choice_, "a", 1, "u0x61");
  g_check_parse_ok(choice_, "b", 1, "u0x62");
  g_check_parse_failed(choice_, "c", 1);
}

static void test_butnot(void) {
  const HParser *butnot_1 = h_butnot(h_ch('a'), h_token((const uint8_t*)"ab", 2));
  const HParser *butnot_2 = h_butnot(h_ch_range('0', '9'), h_ch('6'));

  g_check_parse_ok(butnot_1, "a", 1, "u0x61");
  g_check_parse_failed(butnot_1, "ab", 2);
  g_check_parse_ok(butnot_1, "aa", 2, "u0x61");
  g_check_parse_failed(butnot_2, "6", 1);
}

static void test_difference(void) {
  const HParser *difference_ = h_difference(h_token((const uint8_t*)"ab", 2), h_ch('a'));

  g_check_parse_ok(difference_, "ab", 2, "<61.62>");
  g_check_parse_failed(difference_, "a", 1);
}

static void test_xor(void) {
  const HParser *xor_ = h_xor(h_ch_range('0', '6'), h_ch_range('5', '9'));

  g_check_parse_ok(xor_, "0", 1, "u0x30");
  g_check_parse_ok(xor_, "9", 1, "u0x39");
  g_check_parse_failed(xor_, "5", 1);
  g_check_parse_failed(xor_, "a", 1);
}

static void test_many(void) {
  const HParser *many_ = h_many(h_choice(h_ch('a'), h_ch('b'), NULL));
  g_check_parse_ok(many_, "adef", 4, "(u0x61)");
  g_check_parse_ok(many_, "bdef", 4, "(u0x62)");
  g_check_parse_ok(many_, "aabbabadef", 10, "(u0x61 u0x61 u0x62 u0x62 u0x61 u0x62 u0x61)");
  g_check_parse_ok(many_, "daabbabadef", 11, "()");
}

static void test_many1(void) {
  const HParser *many1_ = h_many1(h_choice(h_ch('a'), h_ch('b'), NULL));

  g_check_parse_ok(many1_, "adef", 4, "(u0x61)");
  g_check_parse_ok(many1_, "bdef", 4, "(u0x62)");
  g_check_parse_ok(many1_, "aabbabadef", 10, "(u0x61 u0x61 u0x62 u0x62 u0x61 u0x62 u0x61)");
  g_check_parse_failed(many1_, "daabbabadef", 11);  
}

static void test_repeat_n(void) {
  const HParser *repeat_n_ = h_repeat_n(h_choice(h_ch('a'), h_ch('b'), NULL), 2);

  g_check_parse_failed(repeat_n_, "adef", 4);
  g_check_parse_ok(repeat_n_, "abdef", 5, "(u0x61 u0x62)");
  g_check_parse_failed(repeat_n_, "dabdef", 6);
}

static void test_optional(void) {
  const HParser *optional_ = h_sequence(h_ch('a'), h_optional(h_choice(h_ch('b'), h_ch('c'), NULL)), h_ch('d'), NULL);
  
  g_check_parse_ok(optional_, "abd", 3, "(u0x61 u0x62 u0x64)");
  g_check_parse_ok(optional_, "acd", 3, "(u0x61 u0x63 u0x64)");
  g_check_parse_ok(optional_, "ad", 2, "(u0x61 null u0x64)");
  g_check_parse_failed(optional_, "aed", 3);
  g_check_parse_failed(optional_, "ab", 2);
  g_check_parse_failed(optional_, "ac", 2);
}

static void test_ignore(void) {
  const HParser *ignore_ = h_sequence(h_ch('a'), h_ignore(h_ch('b')), h_ch('c'), NULL);

  g_check_parse_ok(ignore_, "abc", 3, "(u0x61 u0x63)");
  g_check_parse_failed(ignore_, "ac", 2);
}

static void test_sepBy1(void) {
  const HParser *sepBy1_ = h_sepBy1(h_choice(h_ch('1'), h_ch('2'), h_ch('3'), NULL), h_ch(','));

  g_check_parse_ok(sepBy1_, "1,2,3", 5, "(u0x31 u0x32 u0x33)");
  g_check_parse_ok(sepBy1_, "1,3,2", 5, "(u0x31 u0x33 u0x32)");
  g_check_parse_ok(sepBy1_, "1,3", 3, "(u0x31 u0x33)");
  g_check_parse_ok(sepBy1_, "3", 1, "(u0x33)");
}

static void test_epsilon_p(void) {
  const HParser *epsilon_p_1 = h_sequence(h_ch('a'), h_epsilon_p(), h_ch('b'), NULL);
  const HParser *epsilon_p_2 = h_sequence(h_epsilon_p(), h_ch('a'), NULL);
  const HParser *epsilon_p_3 = h_sequence(h_ch('a'), h_epsilon_p(), NULL);
  
  g_check_parse_ok(epsilon_p_1, "ab", 2, "(u0x61 u0x62)");
  g_check_parse_ok(epsilon_p_2, "a", 1, "(u0x61)");
  g_check_parse_ok(epsilon_p_3, "a", 1, "(u0x61)");
}

static void test_attr_bool(void) {

}

static void test_and(void) {
  const HParser *and_1 = h_sequence(h_and(h_ch('0')), h_ch('0'), NULL);
  const HParser *and_2 = h_sequence(h_and(h_ch('0')), h_ch('1'), NULL);
  const HParser *and_3 = h_sequence(h_ch('1'), h_and(h_ch('2')), NULL);

  g_check_parse_ok(and_1, "0", 1, "(u0x30)");
  g_check_parse_failed(and_2, "0", 1);
  g_check_parse_ok(and_3, "12", 2, "(u0x31)");
}

static void test_not(void) {
  const HParser *not_1 = h_sequence(h_ch('a'), h_choice(h_ch('+'), h_token((const uint8_t*)"++", 2), NULL), h_ch('b'), NULL);
  const HParser *not_2 = h_sequence(h_ch('a'),
				   h_choice(h_sequence(h_ch('+'), h_not(h_ch('+')), NULL),
					  h_token((const uint8_t*)"++", 2),
					  NULL), h_ch('b'), NULL);

  g_check_parse_ok(not_1, "a+b", 3, "(u0x61 u0x2b u0x62)");
  g_check_parse_failed(not_1, "a++b", 4);
  g_check_parse_ok(not_2, "a+b", 3, "(u0x61 (u0x2b) u0x62)");
  g_check_parse_ok(not_2, "a++b", 4, "(u0x61 <2b.2b> u0x62)");
}

void register_parser_tests(void) {
  g_test_add_func("/core/parser/token", test_token);
  g_test_add_func("/core/parser/ch", test_ch);
  g_test_add_func("/core/parser/ch_range", test_ch_range);
  g_test_add_func("/core/parser/int64", test_int64);
  g_test_add_func("/core/parser/int32", test_int32);
  g_test_add_func("/core/parser/int16", test_int16);
  g_test_add_func("/core/parser/int8", test_int8);
  g_test_add_func("/core/parser/uint64", test_uint64);
  g_test_add_func("/core/parser/uint32", test_uint32);
  g_test_add_func("/core/parser/uint16", test_uint16);
  g_test_add_func("/core/parser/uint8", test_uint8);
  g_test_add_func("/core/parser/int_range", test_int_range);
#if 0
  g_test_add_func("/core/parser/float64", test_float64);
  g_test_add_func("/core/parser/float32", test_float32);
#endif
  g_test_add_func("/core/parser/whitespace", test_whitespace);
  g_test_add_func("/core/parser/left", test_left);
  g_test_add_func("/core/parser/right", test_right);
  g_test_add_func("/core/parser/middle", test_middle);
  g_test_add_func("/core/parser/action", test_action);
  g_test_add_func("/core/parser/in", test_in);
  g_test_add_func("/core/parser/not_in", test_not_in);
  g_test_add_func("/core/parser/end_p", test_end_p);
  g_test_add_func("/core/parser/nothing_p", test_nothing_p);
  g_test_add_func("/core/parser/sequence", test_sequence);
  g_test_add_func("/core/parser/choice", test_choice);
  g_test_add_func("/core/parser/butnot", test_butnot);
  g_test_add_func("/core/parser/difference", test_difference);
  g_test_add_func("/core/parser/xor", test_xor);
  g_test_add_func("/core/parser/many", test_many);
  g_test_add_func("/core/parser/many1", test_many1);
  g_test_add_func("/core/parser/repeat_n", test_repeat_n);
  g_test_add_func("/core/parser/optional", test_optional);
  g_test_add_func("/core/parser/sepBy1", test_sepBy1);
  g_test_add_func("/core/parser/epsilon_p", test_epsilon_p);
  g_test_add_func("/core/parser/attr_bool", test_attr_bool);
  g_test_add_func("/core/parser/and", test_and);
  g_test_add_func("/core/parser/not", test_not);
  g_test_add_func("/core/parser/ignore", test_ignore);
}

#endif // #ifdef INCLUDE_TESTS
