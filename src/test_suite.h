/* Test suite for Hammer.
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

#ifndef HAMMER_TEST_SUITE__H
#define HAMMER_TEST_SUITE__H
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>

#include <hammer/internal.h>

// Equivalent to g_assert_*, but not using g_assert...
#define g_check_inttype(fmt, typ, n1, op, n2) do {				\
    typ _n1 = (n1);							\
    typ _n2 = (n2);							\
    if (!(_n1 op _n2)) {						\
      g_test_message("Check failed: (%s): (" fmt " %s " fmt ")",	\
		     #n1 " " #op " " #n2,				\
		     _n1, #op, _n2);					\
      g_test_fail();							\
    }									\
  } while(0)

#define g_check_bytes(len, n1, op, n2) do {	\
    const uint8_t *_n1 = (n1);			\
    const uint8_t *_n2 = (n2);			\
    if (!(memcmp(_n1, _n2, len) op 0)) {	\
      g_test_message("Check failed: (%s)",    	\
		     #n1 " " #op " " #n2);	\
      g_test_fail();				\
    }						\
  } while(0)

#define g_check_string(n1, op, n2) do {			\
    const char *_n1 = (n1);				\
    const char *_n2 = (n2);				\
    if (!(strcmp(_n1, _n2) op 0)) {			\
      g_test_message("Check failed: (%s) (%s %s %s)",	\
		     #n1 " " #op " " #n2,		\
		     _n1, #op, _n2);			\
      g_test_fail();					\
    }							\
  } while(0)

#define g_check_regular(lang) do {			\
    if (!lang->isValidRegular(lang->env)) {		\
      g_test_message("Language is not regular");	\
      g_test_fail();					\
    }							\
  } while(0)

#define g_check_contextfree(lang) do {			\
    if (!lang->isValidCF(lang->env)) {			\
      g_test_message("Language is not context-free");	\
      g_test_fail();					\
    }							\
  } while(0)

#define g_check_compilable(lang, backend, params) do {	\
    if (!h_compile(lang, backend, params)) {		\
      g_test_message("Language is not %s(%s)", #backend, params);	\
      g_test_fail();					\
    }							\
  } while(0)

  
#define g_check_parse_failed__m(mm__, parser, backend, input, inp_len) do { \
    int skip = h_compile__m(mm__, (HParser *)(parser), (HParserBackend)backend, NULL); \
    if(skip != 0) {	\
      g_test_message("Compile failed");					\
      g_test_fail();							\
      break;	\
    }	\
    HParseResult *result = h_parse__m(mm__, parser, (const uint8_t*)input, inp_len); \
    if (NULL != result) {						\
      h_parse_result_free(result);					\
      g_test_message("Check failed: shouldn't have succeeded, but did"); \
      g_test_fail();							\
    }									\
  } while(0)

#define g_check_parse_failed(p, be, input, len)				\
    g_check_parse_failed__m(&system_allocator, p, be, input, len)

#define g_check_parse_ok(parser, backend, input, inp_len) do {		\
    int skip = h_compile((HParser *)(parser), (HParserBackend) backend, NULL); \
    if(skip) {								\
      g_test_message("Compile failed");					\
      g_test_fail();							\
      break;								\
    }									\
    HParseResult *res = h_parse(parser, (const uint8_t*)input, inp_len); \
    if (!res) {								\
      g_test_message("Parse failed on line %d", __LINE__);		\
      g_test_fail();							\
    } else {								\
      HArenaStats stats;						\
      h_allocator_stats(res->arena, &stats);				\
      g_test_message("Parse used %zd bytes, wasted %zd bytes. "		\
                     "Inefficiency: %5f%%",				\
		     stats.used, stats.wasted,				\
		     stats.wasted * 100. / (stats.used+stats.wasted));	\
      h_parse_result_free(res);						\
    }									\
  } while(0)

#define g_check_parse_match(parser, backend, input, inp_len, result) do { \
    int skip = h_compile((HParser *)(parser), (HParserBackend) backend, NULL); \
    if(skip) {								\
      g_test_message("Compile failed");					\
      g_test_fail();							\
      break;								\
    }									\
    HParseResult *res = h_parse(parser, (const uint8_t*)input, inp_len); \
    if (!res) {								\
      g_test_message("Parse failed on line %d", __LINE__);		\
      g_test_fail();							\
    } else {								\
      char* cres = h_write_result_unamb(res->ast);			\
      g_check_string(cres, ==, result);					\
      (&system_allocator)->free(&system_allocator, cres);		\
      HArenaStats stats;						\
      h_allocator_stats(res->arena, &stats);				\
      g_test_message("Parse used %zd bytes, wasted %zd bytes. "		\
                     "Inefficiency: %5f%%",				\
		     stats.used, stats.wasted,				\
		     stats.wasted * 100. / (stats.used+stats.wasted));	\
      h_parse_result_free(res);						\
    }									\
  } while(0)

#define g_check_parse_chunks_failed__m(mm__, parser, backend, chunk1, c1_len, chunk2, c2_len) do { \
    int skip = h_compile__m(mm__, (HParser *)(parser), (HParserBackend)backend, NULL); \
    if(skip) {								\
      g_test_message("Compile failed");					\
      g_test_fail();							\
      break;								\
    }									\
    g_check_parse_chunks_failed___m(mm__, parser, chunk1, c1_len, chunk2, c2_len); \
  } while(0)

#define g_check_parse_chunks_failed___m(mm__, parser, chunk1, c1_len, chunk2, c2_len) do { \
    HSuspendedParser *s = h_parse_start__m(mm__, parser);		\
    if(!s) {								\
      g_test_message("Chunk-wise parsing not available");		\
      g_test_fail();							\
      break;								\
    }									\
    h_parse_chunk(s, (const uint8_t*)chunk1, c1_len);			\
    h_parse_chunk(s, (const uint8_t*)chunk2, c2_len);			\
    HParseResult *res = h_parse_finish(s);				\
    if (NULL != res) {							\
      h_parse_result_free(res);						\
      g_test_message("Check failed: shouldn't have succeeded, but did"); \
      g_test_fail();							\
    }									\
  } while(0)

#define g_check_parse_chunks_failed(p, be, c1, c1_len, c2, c2_len) \
  g_check_parse_chunks_failed__m(&system_allocator, p, be, c1, c1_len, c2, c2_len)

#define g_check_parse_chunks_failed_(p, c1, c1_len, c2, c2_len) \
  g_check_parse_chunks_failed___m(&system_allocator, p, c1, c1_len, c2, c2_len)

#define g_check_parse_chunks_ok(parser, backend, chunk1, c1_len, chunk2, c2_len) do {	\
    int skip = h_compile((HParser *)(parser), (HParserBackend)backend, NULL); \
    if(skip) {								\
      g_test_message("Compile failed");					\
      g_test_fail();							\
      break;								\
    }									\
    g_check_parse_chunks_ok_(parser, chunk1, c1_len, chunk2, c2_len); \
  } while(0)

#define g_check_parse_chunks_ok_(parser, chunk1, c1_len, chunk2, c2_len) do {	\
    HSuspendedParser *s = h_parse_start(parser);			\
    if(!s) {								\
      g_test_message("Chunk-wise parsing not available");		\
      g_test_fail();							\
      break;								\
    }									\
    h_parse_chunk(s, (const uint8_t*)chunk1, c1_len);			\
    h_parse_chunk(s, (const uint8_t*)chunk2, c2_len);			\
    HParseResult *res = h_parse_finish(s);				\
    if (!res) {								\
      g_test_message("Parse failed on line %d", __LINE__);		\
      g_test_fail();							\
    } else {								\
      HArenaStats stats;						\
      h_allocator_stats(res->arena, &stats);				\
      g_test_message("Parse used %zd bytes, wasted %zd bytes. "		\
                     "Inefficiency: %5f%%",				\
		     stats.used, stats.wasted,				\
		     stats.wasted * 100. / (stats.used+stats.wasted));	\
      h_parse_result_free(res);						\
    }									\
  } while(0)

#define g_check_parse_chunks_match(parser, backend, chunk1, c1_len, chunk2, c2_len, result) do { \
    int skip = h_compile((HParser *)(parser), (HParserBackend) backend, NULL); \
    if(skip) {								\
      g_test_message("Compile failed");					\
      g_test_fail();							\
      break;								\
    }									\
    g_check_parse_chunks_match_(parser, chunk1, c1_len, chunk2, c2_len, result); \
  } while(0)

#define g_check_parse_chunks_match_(parser, chunk1, c1_len, chunk2, c2_len, result) do { \
    HSuspendedParser *s = h_parse_start(parser);			\
    if(!s) {								\
      g_test_message("Chunk-wise parsing not available");		\
      g_test_fail();							\
      break;								\
    }									\
    h_parse_chunk(s, (const uint8_t*)chunk1, c1_len);			\
    h_parse_chunk(s, (const uint8_t*)chunk2, c2_len);			\
    HParseResult *res = h_parse_finish(s);				\
    if (!res) {								\
      g_test_message("Parse failed on line %d", __LINE__);		\
      g_test_fail();							\
    } else {								\
      char* cres = h_write_result_unamb(res->ast);			\
      g_check_string(cres, ==, result);					\
      (&system_allocator)->free(&system_allocator, cres);		\
      HArenaStats stats;						\
      h_allocator_stats(res->arena, &stats);				\
      g_test_message("Parse used %zd bytes, wasted %zd bytes. "		\
                     "Inefficiency: %5f%%",				\
		     stats.used, stats.wasted,				\
		     stats.wasted * 100. / (stats.used+stats.wasted));	\
      h_parse_result_free(res);						\
    }									\
  } while(0)

#define g_check_hashtable_present(table, key) do {			\
    if(!h_hashtable_present(table, key)) {				\
      g_test_message("Check failed: key should have been in table, but wasn't"); \
      g_test_fail();							\
    }									\
  } while(0)

#define g_check_hashtable_absent(table, key) do {			\
    if(h_hashtable_present(table, key)) {				\
      g_test_message("Check failed: key shouldn't have been in table, but was"); \
      g_test_fail();							\
    }									\
  } while(0)

#define g_check_hashtable_size(table, n) do {				\
    size_t expected = n;						\
    size_t actual = (table)->used;					\
    if(actual != expected) {						\
      g_test_message("Check failed: table size should have been %zu, but was %zu", \
		     expected, actual);					\
      g_test_fail();							\
    }									\
  } while(0)

#define g_check_stringmap_present(table, key) do {			\
    bool end = (key[strlen(key)-1] == '$');				\
    if(!h_stringmap_present(table, (uint8_t *)key, strlen(key), end)) {	\
      g_test_message("Check failed: \"%s\" should have been in map, but wasn't", key); \
      g_test_fail();							\
    }									\
  } while(0)

#define g_check_stringmap_absent(table, key) do {			\
    bool end = (key[strlen(key)-1] == '$');				\
    if(h_stringmap_present(table, (uint8_t *)key, strlen(key), end)) {	\
      g_test_message("Check failed: \"%s\" shouldn't have been in map, but was", key); \
      g_test_fail();							\
    }									\
  } while(0)


// This stuff needs to be made internal-only; it has no use in user-level test suites
#define g_check_terminal(grammar, parser) \
  g_check_hashtable_absent(grammar->nts, h_desugar(&system_allocator, NULL, parser))

#define g_check_nonterminal(grammar, parser) \
  g_check_hashtable_present(grammar->nts, h_desugar(&system_allocator, NULL, parser))

#define g_check_derives_epsilon(grammar, parser) \
  g_check_hashtable_present(grammar->geneps, h_desugar(&system_allocator, NULL, parser))

#define g_check_derives_epsilon_not(grammar, parser) \
  g_check_hashtable_absent(grammar->geneps, h_desugar(&system_allocator, NULL, parser))

#define g_check_firstset_present(k, grammar, parser, str) \
  g_check_stringmap_present(h_first(k, grammar, h_desugar(&system_allocator, NULL, parser)), str)

#define g_check_firstset_absent(k, grammar, parser, str) \
  g_check_stringmap_absent(h_first(k, grammar, h_desugar(&system_allocator, NULL, parser)), str)

#define g_check_followset_present(k, grammar, parser, str) \
  g_check_stringmap_present(h_follow(k, grammar, h_desugar(&system_allocator, NULL,  parser)), str)

#define g_check_followset_absent(k, grammar, parser, str) \
  g_check_stringmap_absent(h_follow(k, grammar, h_desugar(&system_allocator, NULL, parser)), str)




#define g_check_cmp_int(n1, op, n2) g_check_inttype("%d", int, n1, op, n2)
#define g_check_cmp_int32(n1, op, n2) g_check_inttype("%d", int32_t, n1, op, n2)
#define g_check_cmp_int64(n1, op, n2) g_check_inttype("%" PRId64, int64_t, n1, op, n2)
#define g_check_cmp_uint32(n1, op, n2) g_check_inttype("%u", uint32_t, n1, op, n2)
#define g_check_cmp_uint64(n1, op, n2) g_check_inttype("%" PRIu64, uint64_t, n1, op, n2)
#define g_check_cmpfloat(n1, op, n2) g_check_inttype("%g", float, n1, op, n2)
#define g_check_cmpdouble(n1, op, n2) g_check_inttype("%g", double, n1, op, n2)



#endif // #ifndef HAMMER_TEST_SUITE__H
