#include <glib.h>
#include <string.h>
#include "hammer.h"
#include "glue.h"
#include "internal.h"
#include "test_suite.h"
#include "parsers/parser_internal.h"

static void test_token(gconstpointer backend) {
  const HParser *token_ = h_token((const uint8_t*)"95\xa2", 3);

  g_check_parse_match(token_, (HParserBackend)GPOINTER_TO_INT(backend), "95\xa2", 3, "<39.35.a2>");
  g_check_parse_failed(token_, (HParserBackend)GPOINTER_TO_INT(backend), "95", 2);
}

static void test_ch(gconstpointer backend) {
  const HParser *ch_ = h_ch(0xa2);

  g_check_parse_match(ch_, (HParserBackend)GPOINTER_TO_INT(backend), "\xa2", 1, "u0xa2");
  g_check_parse_failed(ch_, (HParserBackend)GPOINTER_TO_INT(backend), "\xa3", 1);
}

static void test_ch_range(gconstpointer backend) {
  const HParser *range_ = h_ch_range('a', 'c');

  g_check_parse_match(range_, (HParserBackend)GPOINTER_TO_INT(backend), "b", 1, "u0x62");
  g_check_parse_failed(range_, (HParserBackend)GPOINTER_TO_INT(backend), "d", 1);
}

//@MARK_START
static void test_int64(gconstpointer backend) {
  const HParser *int64_ = h_int64();

  g_check_parse_match(int64_, (HParserBackend)GPOINTER_TO_INT(backend), "\xff\xff\xff\xfe\x00\x00\x00\x00", 8, "s-0x200000000");
  g_check_parse_failed(int64_, (HParserBackend)GPOINTER_TO_INT(backend), "\xff\xff\xff\xfe\x00\x00\x00", 7);
}

static void test_int32(gconstpointer backend) {
  const HParser *int32_ = h_int32();

  g_check_parse_match(int32_, (HParserBackend)GPOINTER_TO_INT(backend), "\xff\xfe\x00\x00", 4, "s-0x20000");
  g_check_parse_failed(int32_, (HParserBackend)GPOINTER_TO_INT(backend), "\xff\xfe\x00", 3);

  g_check_parse_match(int32_, (HParserBackend)GPOINTER_TO_INT(backend), "\x00\x02\x00\x00", 4, "s0x20000");
  g_check_parse_failed(int32_, (HParserBackend)GPOINTER_TO_INT(backend), "\x00\x02\x00", 3);
}

static void test_int16(gconstpointer backend) {
  const HParser *int16_ = h_int16();

  g_check_parse_match(int16_, (HParserBackend)GPOINTER_TO_INT(backend), "\xfe\x00", 2, "s-0x200");
  g_check_parse_failed(int16_, (HParserBackend)GPOINTER_TO_INT(backend), "\xfe", 1);

  g_check_parse_match(int16_, (HParserBackend)GPOINTER_TO_INT(backend), "\x02\x00", 2, "s0x200");
  g_check_parse_failed(int16_, (HParserBackend)GPOINTER_TO_INT(backend), "\x02", 1);
}

static void test_int8(gconstpointer backend) {
  const HParser *int8_ = h_int8();

  g_check_parse_match(int8_, (HParserBackend)GPOINTER_TO_INT(backend), "\x88", 1, "s-0x78");
  g_check_parse_failed(int8_, (HParserBackend)GPOINTER_TO_INT(backend), "", 0);
}

static void test_uint64(gconstpointer backend) {
  const HParser *uint64_ = h_uint64();

  g_check_parse_match(uint64_, (HParserBackend)GPOINTER_TO_INT(backend), "\x00\x00\x00\x02\x00\x00\x00\x00", 8, "u0x200000000");
  g_check_parse_failed(uint64_, (HParserBackend)GPOINTER_TO_INT(backend), "\x00\x00\x00\x02\x00\x00\x00", 7);
}

static void test_uint32(gconstpointer backend) {
  const HParser *uint32_ = h_uint32();

  g_check_parse_match(uint32_, (HParserBackend)GPOINTER_TO_INT(backend), "\x00\x02\x00\x00", 4, "u0x20000");
  g_check_parse_failed(uint32_, (HParserBackend)GPOINTER_TO_INT(backend), "\x00\x02\x00", 3);
}

static void test_uint16(gconstpointer backend) {
  const HParser *uint16_ = h_uint16();

  g_check_parse_match(uint16_, (HParserBackend)GPOINTER_TO_INT(backend), "\x02\x00", 2, "u0x200");
  g_check_parse_failed(uint16_, (HParserBackend)GPOINTER_TO_INT(backend), "\x02", 1);
}

static void test_uint8(gconstpointer backend) {
  const HParser *uint8_ = h_uint8();

  g_check_parse_match(uint8_, (HParserBackend)GPOINTER_TO_INT(backend), "\x78", 1, "u0x78");
  g_check_parse_failed(uint8_, (HParserBackend)GPOINTER_TO_INT(backend), "", 0);
}
//@MARK_END

static void test_int_range(gconstpointer backend) {
  const HParser *int_range_ = h_int_range(h_uint8(), 3, 10);
  
  g_check_parse_match(int_range_, (HParserBackend)GPOINTER_TO_INT(backend), "\x05", 1, "u0x5");
  g_check_parse_failed(int_range_, (HParserBackend)GPOINTER_TO_INT(backend), "\xb", 1);
}

#if 0
static void test_float64(gconstpointer backend) {
  const HParser *float64_ = h_float64();

  g_check_parse_match(float64_, (HParserBackend)GPOINTER_TO_INT(backend), "\x3f\xf0\x00\x00\x00\x00\x00\x00", 8, 1.0);
  g_check_parse_failed(float64_, (HParserBackend)GPOINTER_TO_INT(backend), "\x3f\xf0\x00\x00\x00\x00\x00", 7);
}

static void test_float32(gconstpointer backend) {
  const HParser *float32_ = h_float32();

  g_check_parse_match(float32_, (HParserBackend)GPOINTER_TO_INT(backend), "\x3f\x80\x00\x00", 4, 1.0);
  g_check_parse_failed(float32_, (HParserBackend)GPOINTER_TO_INT(backend), "\x3f\x80\x00");
}
#endif


static void test_whitespace(gconstpointer backend) {
  const HParser *whitespace_ = h_whitespace(h_ch('a'));
  const HParser *whitespace_end = h_whitespace(h_end_p());

  g_check_parse_match(whitespace_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1, "u0x61");
  g_check_parse_match(whitespace_, (HParserBackend)GPOINTER_TO_INT(backend), " a", 2, "u0x61");
  g_check_parse_match(whitespace_, (HParserBackend)GPOINTER_TO_INT(backend), "  a", 3, "u0x61");
  g_check_parse_match(whitespace_, (HParserBackend)GPOINTER_TO_INT(backend), "\ta", 2, "u0x61");
  g_check_parse_failed(whitespace_, (HParserBackend)GPOINTER_TO_INT(backend), "_a", 2);

  g_check_parse_match(whitespace_end, (HParserBackend)GPOINTER_TO_INT(backend), "", 0, "NULL");
  g_check_parse_match(whitespace_end, (HParserBackend)GPOINTER_TO_INT(backend),"  ", 2, "NULL");
  g_check_parse_failed(whitespace_end, (HParserBackend)GPOINTER_TO_INT(backend),"  x", 3);
}

static void test_left(gconstpointer backend) {
  const HParser *left_ = h_left(h_ch('a'), h_ch(' '));

  g_check_parse_match(left_, (HParserBackend)GPOINTER_TO_INT(backend), "a ", 2, "u0x61");
  g_check_parse_failed(left_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1);
  g_check_parse_failed(left_, (HParserBackend)GPOINTER_TO_INT(backend), " ", 1);
  g_check_parse_failed(left_, (HParserBackend)GPOINTER_TO_INT(backend), "ab", 2);
}

static void test_right(gconstpointer backend) {
  const HParser *right_ = h_right(h_ch(' '), h_ch('a'));

  g_check_parse_match(right_, (HParserBackend)GPOINTER_TO_INT(backend), " a", 2, "u0x61");
  g_check_parse_failed(right_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1);
  g_check_parse_failed(right_, (HParserBackend)GPOINTER_TO_INT(backend), " ", 1);
  g_check_parse_failed(right_, (HParserBackend)GPOINTER_TO_INT(backend), "ba", 2);
}

static void test_middle(gconstpointer backend) {
  const HParser *middle_ = h_middle(h_ch(' '), h_ch('a'), h_ch(' '));

  g_check_parse_match(middle_, (HParserBackend)GPOINTER_TO_INT(backend), " a ", 3, "u0x61");
  g_check_parse_failed(middle_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1);
  g_check_parse_failed(middle_, (HParserBackend)GPOINTER_TO_INT(backend), " ", 1);
  g_check_parse_failed(middle_, (HParserBackend)GPOINTER_TO_INT(backend), " a", 2);
  g_check_parse_failed(middle_, (HParserBackend)GPOINTER_TO_INT(backend), "a ", 2);
  g_check_parse_failed(middle_, (HParserBackend)GPOINTER_TO_INT(backend), " b ", 3);
  g_check_parse_failed(middle_, (HParserBackend)GPOINTER_TO_INT(backend), "ba ", 3);
  g_check_parse_failed(middle_, (HParserBackend)GPOINTER_TO_INT(backend), " ab", 3);
}

#include <ctype.h>

HParsedToken* upcase(const HParseResult *p, void* user_data) {
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
      return ret;
    }
  case TT_UINT:
    {
      HParsedToken *ret = a_new_(p->arena, HParsedToken, 1);
      ret->token_type = TT_UINT;
      ret->uint = toupper(p->ast->uint);
      return ret;
    }
  default:
    return (HParsedToken*)p->ast;
  }
}

static void test_action(gconstpointer backend) {
  const HParser *action_ = h_action(h_sequence(h_choice(h_ch('a'), 
							h_ch('A'), 
							NULL), 
					       h_choice(h_ch('b'), 
							h_ch('B'), 
						      NULL), 
					       NULL), 
				    upcase,
				    NULL);
  
  g_check_parse_match(action_, (HParserBackend)GPOINTER_TO_INT(backend), "ab", 2, "(u0x41 u0x42)");
  g_check_parse_match(action_, (HParserBackend)GPOINTER_TO_INT(backend), "AB", 2, "(u0x41 u0x42)");
  g_check_parse_failed(action_, (HParserBackend)GPOINTER_TO_INT(backend), "XX", 2);
}

static void test_in(gconstpointer backend) {
  uint8_t options[3] = { 'a', 'b', 'c' };
  const HParser *in_ = h_in(options, 3);
  g_check_parse_match(in_, (HParserBackend)GPOINTER_TO_INT(backend), "b", 1, "u0x62");
  g_check_parse_failed(in_, (HParserBackend)GPOINTER_TO_INT(backend), "d", 1);

}

static void test_not_in(gconstpointer backend) {
  uint8_t options[3] = { 'a', 'b', 'c' };
  const HParser *not_in_ = h_not_in(options, 3);
  g_check_parse_match(not_in_, (HParserBackend)GPOINTER_TO_INT(backend), "d", 1, "u0x64");
  g_check_parse_failed(not_in_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1);

}

static void test_end_p(gconstpointer backend) {
  const HParser *end_p_ = h_sequence(h_ch('a'), h_end_p(), NULL);
  g_check_parse_match(end_p_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1, "(u0x61)");
  g_check_parse_failed(end_p_, (HParserBackend)GPOINTER_TO_INT(backend), "aa", 2);
}

static void test_nothing_p(gconstpointer backend) {
  const HParser *nothing_p_ = h_nothing_p();
  g_check_parse_failed(nothing_p_,  (HParserBackend)GPOINTER_TO_INT(backend),"a", 1);
}

static void test_sequence(gconstpointer backend) {
  const HParser *sequence_1 = h_sequence(h_ch('a'), h_ch('b'), NULL);
  const HParser *sequence_2 = h_sequence(h_ch('a'), h_whitespace(h_ch('b')), NULL);
  const HParser *sequence_3 = h_sequence(NULL, NULL);   // second NULL is to silence GCC

  g_check_parse_match(sequence_1, (HParserBackend)GPOINTER_TO_INT(backend), "ab", 2, "(u0x61 u0x62)");
  g_check_parse_failed(sequence_1, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1);
  g_check_parse_failed(sequence_1, (HParserBackend)GPOINTER_TO_INT(backend), "b", 1);
  g_check_parse_match(sequence_2, (HParserBackend)GPOINTER_TO_INT(backend), "ab", 2, "(u0x61 u0x62)");
  g_check_parse_match(sequence_2, (HParserBackend)GPOINTER_TO_INT(backend), "a b", 3, "(u0x61 u0x62)");
  g_check_parse_match(sequence_2, (HParserBackend)GPOINTER_TO_INT(backend), "a  b", 4, "(u0x61 u0x62)");  
  g_check_parse_match(sequence_3, (HParserBackend)GPOINTER_TO_INT(backend), "", 0, "()");
}

static void test_choice(gconstpointer backend) {
  const HParser *choice_ = h_choice(h_ch('a'), h_ch('b'), NULL);

  g_check_parse_match(choice_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1, "u0x61");
  g_check_parse_match(choice_, (HParserBackend)GPOINTER_TO_INT(backend), "b", 1, "u0x62");
  g_check_parse_failed(choice_, (HParserBackend)GPOINTER_TO_INT(backend), "c", 1);
}

static void test_butnot(gconstpointer backend) {
  const HParser *butnot_1 = h_butnot(h_ch('a'), h_token((const uint8_t*)"ab", 2));
  const HParser *butnot_2 = h_butnot(h_ch_range('0', '9'), h_ch('6'));

  g_check_parse_match(butnot_1, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1, "u0x61");
  g_check_parse_failed(butnot_1, (HParserBackend)GPOINTER_TO_INT(backend), "ab", 2);
  g_check_parse_match(butnot_1, (HParserBackend)GPOINTER_TO_INT(backend), "aa", 2, "u0x61");
  g_check_parse_failed(butnot_2, (HParserBackend)GPOINTER_TO_INT(backend), "6", 1);
}

static void test_difference(gconstpointer backend) {
  const HParser *difference_ = h_difference(h_token((const uint8_t*)"ab", 2), h_ch('a'));

  g_check_parse_match(difference_, (HParserBackend)GPOINTER_TO_INT(backend), "ab", 2, "<61.62>");
  g_check_parse_failed(difference_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1);
}

static void test_xor(gconstpointer backend) {
  const HParser *xor_ = h_xor(h_ch_range('0', '6'), h_ch_range('5', '9'));

  g_check_parse_match(xor_, (HParserBackend)GPOINTER_TO_INT(backend), "0", 1, "u0x30");
  g_check_parse_match(xor_, (HParserBackend)GPOINTER_TO_INT(backend), "9", 1, "u0x39");
  g_check_parse_failed(xor_, (HParserBackend)GPOINTER_TO_INT(backend), "5", 1);
  g_check_parse_failed(xor_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1);
}

static void test_many(gconstpointer backend) {
  const HParser *many_ = h_many(h_choice(h_ch('a'), h_ch('b'), NULL));

  g_check_parse_match(many_, (HParserBackend)GPOINTER_TO_INT(backend), "", 0, "()");
  g_check_parse_match(many_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1, "(u0x61)");
  //  g_check_parse_match(many_, (HParserBackend)GPOINTER_TO_INT(backend), "adef", 4, "(u0x61)");
  g_check_parse_match(many_, (HParserBackend)GPOINTER_TO_INT(backend), "b", 1, "(u0x62)");
  //  g_check_parse_match(many_, (HParserBackend)GPOINTER_TO_INT(backend), "bdef", 4, "(u0x62)");
  g_check_parse_match(many_, (HParserBackend)GPOINTER_TO_INT(backend), "aabbaba", 7, "(u0x61 u0x61 u0x62 u0x62 u0x61 u0x62 u0x61)");
  //  g_check_parse_match(many_, (HParserBackend)GPOINTER_TO_INT(backend), "aabbabadef", 10, "(u0x61 u0x61 u0x62 u0x62 u0x61 u0x62 u0x61)");
  //  g_check_parse_match(many_, (HParserBackend)GPOINTER_TO_INT(backend), "daabbabadef", 11, "()");
}

static void test_many1(gconstpointer backend) {
  const HParser *many1_ = h_many1(h_choice(h_ch('a'), h_ch('b'), NULL));

  g_check_parse_failed(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "", 0);
  g_check_parse_match(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1, "(u0x61)");
  //  g_check_parse_match(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "adef", 4, "(u0x61)");
  g_check_parse_match(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "b", 1, "(u0x62)");
  //  g_check_parse_match(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "bdef", 4, "(u0x62)");
  g_check_parse_match(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "aabbaba", 7, "(u0x61 u0x61 u0x62 u0x62 u0x61 u0x62 u0x61)");
  //  g_check_parse_match(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "aabbabadef", 10, "(u0x61 u0x61 u0x62 u0x62 u0x61 u0x62 u0x61)");
  g_check_parse_failed(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "daabbabadef", 11);  
}

static void test_repeat_n(gconstpointer backend) {
  const HParser *repeat_n_ = h_repeat_n(h_choice(h_ch('a'), h_ch('b'), NULL), 2);

  g_check_parse_failed(repeat_n_, (HParserBackend)GPOINTER_TO_INT(backend), "adef", 4);
  g_check_parse_match(repeat_n_, (HParserBackend)GPOINTER_TO_INT(backend), "abdef", 5, "(u0x61 u0x62)");
  g_check_parse_failed(repeat_n_, (HParserBackend)GPOINTER_TO_INT(backend), "dabdef", 6);
}

static void test_optional(gconstpointer backend) {
  const HParser *optional_ = h_sequence(h_ch('a'), h_optional(h_choice(h_ch('b'), h_ch('c'), NULL)), h_ch('d'), NULL);
  
  g_check_parse_match(optional_, (HParserBackend)GPOINTER_TO_INT(backend), "abd", 3, "(u0x61 u0x62 u0x64)");
  g_check_parse_match(optional_, (HParserBackend)GPOINTER_TO_INT(backend), "acd", 3, "(u0x61 u0x63 u0x64)");
  g_check_parse_match(optional_, (HParserBackend)GPOINTER_TO_INT(backend), "ad", 2, "(u0x61 null u0x64)");
  g_check_parse_failed(optional_, (HParserBackend)GPOINTER_TO_INT(backend), "aed", 3);
  g_check_parse_failed(optional_, (HParserBackend)GPOINTER_TO_INT(backend), "ab", 2);
  g_check_parse_failed(optional_, (HParserBackend)GPOINTER_TO_INT(backend), "ac", 2);
}

static void test_ignore(gconstpointer backend) {
  const HParser *ignore_ = h_sequence(h_ch('a'), h_ignore(h_ch('b')), h_ch('c'), NULL);

  g_check_parse_match(ignore_, (HParserBackend)GPOINTER_TO_INT(backend), "abc", 3, "(u0x61 u0x63)");
  g_check_parse_failed(ignore_, (HParserBackend)GPOINTER_TO_INT(backend), "ac", 2);
}

static void test_sepBy(gconstpointer backend) {
  const HParser *sepBy_ = h_sepBy(h_choice(h_ch('1'), h_ch('2'), h_ch('3'), NULL), h_ch(','));

  g_check_parse_match(sepBy_, (HParserBackend)GPOINTER_TO_INT(backend), "1,2,3", 5, "(u0x31 u0x32 u0x33)");
  g_check_parse_match(sepBy_, (HParserBackend)GPOINTER_TO_INT(backend), "1,3,2", 5, "(u0x31 u0x33 u0x32)");
  g_check_parse_match(sepBy_, (HParserBackend)GPOINTER_TO_INT(backend), "1,3", 3, "(u0x31 u0x33)");
  g_check_parse_match(sepBy_, (HParserBackend)GPOINTER_TO_INT(backend), "3", 1, "(u0x33)");
  g_check_parse_match(sepBy_, (HParserBackend)GPOINTER_TO_INT(backend), "", 0, "()");
}

static void test_sepBy1(gconstpointer backend) {
  const HParser *sepBy1_ = h_sepBy1(h_choice(h_ch('1'), h_ch('2'), h_ch('3'), NULL), h_ch(','));

  g_check_parse_match(sepBy1_, (HParserBackend)GPOINTER_TO_INT(backend), "1,2,3", 5, "(u0x31 u0x32 u0x33)");
  g_check_parse_match(sepBy1_, (HParserBackend)GPOINTER_TO_INT(backend), "1,3,2", 5, "(u0x31 u0x33 u0x32)");
  g_check_parse_match(sepBy1_, (HParserBackend)GPOINTER_TO_INT(backend), "1,3", 3, "(u0x31 u0x33)");
  g_check_parse_match(sepBy1_, (HParserBackend)GPOINTER_TO_INT(backend), "3", 1, "(u0x33)");
  g_check_parse_failed(sepBy1_, (HParserBackend)GPOINTER_TO_INT(backend), "", 0);
}

static void test_epsilon_p(gconstpointer backend) {
  const HParser *epsilon_p_1 = h_sequence(h_ch('a'), h_epsilon_p(), h_ch('b'), NULL);
  const HParser *epsilon_p_2 = h_sequence(h_epsilon_p(), h_ch('a'), NULL);
  const HParser *epsilon_p_3 = h_sequence(h_ch('a'), h_epsilon_p(), NULL);
  
  g_check_parse_match(epsilon_p_1, (HParserBackend)GPOINTER_TO_INT(backend), "ab", 2, "(u0x61 u0x62)");
  g_check_parse_match(epsilon_p_2, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1, "(u0x61)");
  g_check_parse_match(epsilon_p_3, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1, "(u0x61)");
}

bool validate_test_ab(HParseResult *p, void* user_data) {
  if (TT_SEQUENCE != p->ast->token_type) 
    return false;
  if (TT_UINT != p->ast->seq->elements[0]->token_type)
    return false;
  if (TT_UINT != p->ast->seq->elements[1]->token_type)
    return false;
  return (p->ast->seq->elements[0]->uint == p->ast->seq->elements[1]->uint);
}

static void test_attr_bool(gconstpointer backend) {
  const HParser *ab_ = h_attr_bool(h_many1(h_choice(h_ch('a'), h_ch('b'), NULL)),
				   validate_test_ab,
				   NULL);

  g_check_parse_match(ab_, (HParserBackend)GPOINTER_TO_INT(backend), "aa", 2, "(u0x61 u0x61)");
  g_check_parse_match(ab_, (HParserBackend)GPOINTER_TO_INT(backend), "bb", 2, "(u0x62 u0x62)");
  g_check_parse_failed(ab_, (HParserBackend)GPOINTER_TO_INT(backend), "ab", 2);
}

static void test_and(gconstpointer backend) {
  const HParser *and_1 = h_sequence(h_and(h_ch('0')), h_ch('0'), NULL);
  const HParser *and_2 = h_sequence(h_and(h_ch('0')), h_ch('1'), NULL);
  const HParser *and_3 = h_sequence(h_ch('1'), h_and(h_ch('2')), NULL);

  g_check_parse_match(and_1, (HParserBackend)GPOINTER_TO_INT(backend), "0", 1, "(u0x30)");
  g_check_parse_failed(and_2, (HParserBackend)GPOINTER_TO_INT(backend), "0", 1);
  g_check_parse_match(and_3, (HParserBackend)GPOINTER_TO_INT(backend), "12", 2, "(u0x31)");
}

static void test_not(gconstpointer backend) {
  const HParser *not_1 = h_sequence(h_ch('a'), h_choice(h_ch('+'), h_token((const uint8_t*)"++", 2), NULL), h_ch('b'), NULL);
  const HParser *not_2 = h_sequence(h_ch('a'),
				    h_choice(h_sequence(h_ch('+'), h_not(h_ch('+')), NULL),
					  h_token((const uint8_t*)"++", 2),
					  NULL), h_ch('b'), NULL);

  g_check_parse_match(not_1, (HParserBackend)GPOINTER_TO_INT(backend), "a+b", 3, "(u0x61 u0x2b u0x62)");
  g_check_parse_failed(not_1, (HParserBackend)GPOINTER_TO_INT(backend), "a++b", 4);
  g_check_parse_match(not_2, (HParserBackend)GPOINTER_TO_INT(backend), "a+b", 3, "(u0x61 (u0x2b) u0x62)");
  g_check_parse_match(not_2, (HParserBackend)GPOINTER_TO_INT(backend), "a++b", 4, "(u0x61 <2b.2b> u0x62)");
}

static void test_leftrec(gconstpointer backend) {
  HParser *a_ = h_ch('a');

  HParser *lr_ = h_indirect();
  h_bind_indirect(lr_, h_choice(h_sequence(lr_, a_, NULL), h_epsilon_p(), NULL));

  g_check_parse_match(lr_, (HParserBackend)GPOINTER_TO_INT(backend), "", 0, "NULL");
  g_check_parse_match(lr_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1, "(u0x61)");
  g_check_parse_match(lr_, (HParserBackend)GPOINTER_TO_INT(backend), "aa", 2, "((u0x61) u0x61)");
  g_check_parse_match(lr_, (HParserBackend)GPOINTER_TO_INT(backend), "aaa", 3, "(((u0x61) u0x61) u0x61)");
}

static void test_leftrec_ne(gconstpointer backend) {
  HParser *a_ = h_ch('a');

  HParser *lr_ = h_indirect();
  h_bind_indirect(lr_, h_choice(h_sequence(lr_, a_, NULL), a_, NULL));

  g_check_parse_match(lr_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1, "u0x61");
  g_check_parse_match(lr_, (HParserBackend)GPOINTER_TO_INT(backend), "aa", 2, "(u0x61 u0x61)");
  g_check_parse_match(lr_, (HParserBackend)GPOINTER_TO_INT(backend), "aaa", 3, "((u0x61 u0x61) u0x61)");
  g_check_parse_failed(lr_, (HParserBackend)GPOINTER_TO_INT(backend), "", 0);
}

static void test_rightrec(gconstpointer backend) {
  HParser *a_ = h_ch('a');

  HParser *rr_ = h_indirect();
  h_bind_indirect(rr_, h_choice(h_sequence(a_, rr_, NULL), h_epsilon_p(), NULL));

  g_check_parse_match(rr_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1, "(u0x61)");
  g_check_parse_match(rr_, (HParserBackend)GPOINTER_TO_INT(backend), "aa", 2, "(u0x61 (u0x61))");
  g_check_parse_match(rr_, (HParserBackend)GPOINTER_TO_INT(backend), "aaa", 3, "(u0x61 (u0x61 (u0x61)))");
}

static void test_iterative(gconstpointer backend) {
  HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
  HParser *p;

  p = h_token((uint8_t*)"foobar", 6);
  g_check_parse_chunks_match(p, be, "foo",3, "bar",3, "<66.6f.6f.62.61.72>");
  g_check_parse_chunks_match(p, be, "foo",3, "barbaz",6, "<66.6f.6f.62.61.72>");
  g_check_parse_chunks_failed(p, be, "fou",3, "bar",3);
  g_check_parse_chunks_failed(p, be, "foo",3, "par",3);
  g_check_parse_chunks_failed(p, be, "foo",3, "baz",3);

  p = h_sequence(h_ch('f'), h_token((uint8_t*)"ooba", 4), h_ch('r'), NULL);
  g_check_parse_chunks_match(p, be, "foo",3, "bar",3, "(u0x66 <6f.6f.62.61> u0x72)");
  g_check_parse_chunks_match(p, be, "foo",3, "barbaz",6, "(u0x66 <6f.6f.62.61> u0x72)");
  g_check_parse_chunks_failed(p, be, "fou",3, "bar",3);
  g_check_parse_chunks_failed(p, be, "foo",3, "par",3);
  g_check_parse_chunks_failed(p, be, "foo",3, "baz",3);

  p = h_choice(h_token((uint8_t*)"foobar", 6),
               h_token((uint8_t*)"phupar", 6), NULL);
  g_check_parse_chunks_match(p, be, "foo",3, "bar",3, "<66.6f.6f.62.61.72>");
  g_check_parse_chunks_match(p, be, "foo",3, "barbaz",6, "<66.6f.6f.62.61.72>");
  g_check_parse_chunks_match(p, be, "phu",3, "par",3, "<70.68.75.70.61.72>");
  g_check_parse_chunks_failed(p, be, "fou",3, "bar",3);
  g_check_parse_chunks_failed(p, be, "foo",3, "baz",3);
  g_check_parse_chunks_match(p, be, "foobar",6, "",0, "<66.6f.6f.62.61.72>");
  g_check_parse_chunks_match(p, be, "",0, "foobar",6, "<66.6f.6f.62.61.72>");
  g_check_parse_chunks_failed(p, be, "foo",3, "",0);
  g_check_parse_chunks_failed(p, be, "",0, "foo",3);

  p = h_sequence(h_ch('f'), h_choice(h_token((uint8_t*)"oo", 2),
                                     h_token((uint8_t*)"uu", 2), NULL), NULL);
  g_check_parse_chunks_match(p, be, "f",1, "oo",2, "(u0x66 <6f.6f>)");
  g_check_parse_chunks_match(p, be, "f",1, "uu",2, "(u0x66 <75.75>)");
  g_check_parse_chunks_failed(p, be, "g",1, "oo",2);
  g_check_parse_chunks_failed(p, be, "f",1, "ou",2);
  g_check_parse_chunks_failed(p, be, "f",1, "uo",2);
}

static void test_iterative_lookahead(gconstpointer backend) {
  HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
  HParser *p;

  // needs 2 lookahead
  p = h_sequence(h_ch('f'), h_choice(h_token((uint8_t*)"oo", 2),
                                     h_token((uint8_t*)"ou", 2), NULL), NULL);
  if(h_compile(p, be, (void *)2) != 0) {
    g_test_message("Compile failed");
    g_test_fail();
    return;
  }

  // partial chunk consumed
  g_check_parse_chunks_match_(p, "fo",2, "o",1, "(u0x66 <6f.6f>)");
  g_check_parse_chunks_match_(p, "fo",2, "u",1, "(u0x66 <6f.75>)");
  g_check_parse_chunks_failed_(p, "go",2, "o",1);
  g_check_parse_chunks_failed_(p, "fa",2, "u",1);
  g_check_parse_chunks_failed_(p, "fo",2, "b",1);
}

static void test_iterative_result_length(gconstpointer backend) {
  HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
  HParser *p = h_token((uint8_t*)"foobar", 6);

  if(h_compile(p, be, NULL) != 0) {
    g_test_message("Compile failed");
    g_test_fail();
    return;
  }

  HSuspendedParser *s = h_parse_start(p);
  if(!s) {
    g_test_message("Chunked parsing not available");
    g_test_fail();
    return;
  }
  h_parse_chunk(s, (uint8_t*)"foo", 3);
  h_parse_chunk(s, (uint8_t*)"ba", 2);
  h_parse_chunk(s, (uint8_t*)"rbaz", 4);
  HParseResult *r = h_parse_finish(s);
  if(!r) {
    g_test_message("Parse failed");
    g_test_fail();
    return;
  }

  g_check_cmp_int64(r->bit_length, ==, 48);
}

static void test_result_length(gconstpointer backend) {
  HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
  HParser *p = h_token((uint8_t*)"foo", 3);

  if(h_compile(p, be, NULL) != 0) {
    g_test_message("Compile failed");
    g_test_fail();
    return;
  }

  HParseResult *r = h_parse(p, (uint8_t*)"foobar", 6);
  if(!r) {
    g_test_message("Parse failed");
    g_test_fail();
    return;
  }

  g_check_cmp_int64(r->bit_length, ==, 24);
}

static void test_token_position(gconstpointer backend) {
  HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
  HParser *p = h_sequence(h_token((uint8_t*)"foo",3),
                          h_token((uint8_t*)"bar",3), NULL);

  if(h_compile(p, be, NULL) != 0) {
    g_test_message("Compile failed");
    g_test_fail();
    return;
  }

  HParseResult *r = h_parse(p, (uint8_t*)"foobar", 6);
  if(!r) {
    g_test_message("Parse failed");
    g_test_fail();
    return;
  }

  assert(r->ast != NULL);
  HParsedToken *foo = H_INDEX_TOKEN(r->ast, 0);
  HParsedToken *bar = H_INDEX_TOKEN(r->ast, 1);

  g_check_cmp_uint64(foo->index, ==, 0);
  g_check_cmp_uint64(foo->bit_offset, ==, 0);
  g_check_cmp_uint64(bar->index, ==, 3);
  g_check_cmp_uint64(bar->bit_offset, ==, 0);
}

static void test_ambiguous(gconstpointer backend) {
  HParser *d_ = h_ch('d');
  HParser *p_ = h_ch('+');
  HParser *E_ = h_indirect();
  h_bind_indirect(E_, h_choice(h_sequence(E_, p_, E_, NULL), d_, NULL));
  HParser *expr_ = h_action(E_, h_act_flatten, NULL);

  g_check_parse_match(expr_, (HParserBackend)GPOINTER_TO_INT(backend), "d", 1, "(u0x64)");
  g_check_parse_match(expr_, (HParserBackend)GPOINTER_TO_INT(backend), "d+d", 3, "(u0x64 u0x2b u0x64)");
  g_check_parse_match(expr_, (HParserBackend)GPOINTER_TO_INT(backend), "d+d+d", 5, "(u0x64 u0x2b u0x64 u0x2b u0x64)");
  g_check_parse_failed(expr_, (HParserBackend)GPOINTER_TO_INT(backend), "d+", 2);
}

static void test_endianness(gconstpointer backend) {
  HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);

  HParser *u32_ = h_uint32();
  HParser *u5_ = h_bits(5, false);

  char bb = BYTE_BIG_ENDIAN | BIT_BIG_ENDIAN;
  char bl = BYTE_BIG_ENDIAN | BIT_LITTLE_ENDIAN;
  char lb = BYTE_LITTLE_ENDIAN | BIT_BIG_ENDIAN;
  char ll = BYTE_LITTLE_ENDIAN | BIT_LITTLE_ENDIAN;

  HParser *bb_u32_ = h_with_endianness(bb, u32_);
  HParser *bb_u5_ = h_with_endianness(bb, u5_);
  HParser *ll_u32_ = h_with_endianness(ll, u32_);
  HParser *ll_u5_ = h_with_endianness(ll, u5_);
  HParser *bl_u32_ = h_with_endianness(bl, u32_);
  HParser *bl_u5_ = h_with_endianness(bl, u5_);
  HParser *lb_u32_ = h_with_endianness(lb, u32_);
  HParser *lb_u5_ = h_with_endianness(lb, u5_);

  // default: big-endian
  g_check_parse_match(u32_, be, "abcd", 4, "u0x61626364");
  g_check_parse_match(u5_,  be, "abcd", 4, "u0xc");		// 0x6 << 1

  // both big-endian
  g_check_parse_match(bb_u32_, be, "abcd", 4, "u0x61626364");
  g_check_parse_match(bb_u5_,  be, "abcd", 4, "u0xc");		// 0x6 << 1

  // both little-endian
  g_check_parse_match(ll_u32_, be, "abcd", 4, "u0x64636261");
  g_check_parse_match(ll_u5_,  be, "abcd", 4, "u0x1");

  // mixed cases
  g_check_parse_match(bl_u32_, be, "abcd", 4, "u0x61626364");
  g_check_parse_match(bl_u5_,  be, "abcd", 4, "u0x1");
  g_check_parse_match(lb_u32_, be, "abcd", 4, "u0x64636261");
  g_check_parse_match(lb_u5_,  be, "abcd", 4, "u0xc");
}

HParsedToken* act_get(const HParseResult *p, void* user_data) {
  HParsedToken *ret = a_new_(p->arena, HParsedToken, 1);
  ret->token_type = TT_UINT;
  ret->uint = 3 * (1 << p->ast->uint);
  return ret;
}

static void test_put_get(gconstpointer backend) {
  HParser *p = h_sequence(h_put_value(h_uint8(), "size"),
			  h_token((const uint8_t*)"foo", 3),
			  h_length_value(h_action(h_get_value("size"),
						  act_get, NULL),
					 h_uint8()),
			  NULL);
  // Yes, the quotes in the next line look weird. Leave them alone,
  // this is to deal with how C strings handle hex-formatted chars.
  g_check_parse_match(p, (HParserBackend)GPOINTER_TO_INT(backend), "\x01""fooabcdef", 10, "(u0x1 <66.6f.6f> (u0x61 u0x62 u0x63 u0x64 u0x65 u0x66))");
  g_check_parse_failed(p, (HParserBackend)GPOINTER_TO_INT(backend), "\x01""fooabcde", 9);
}

static void test_permutation(gconstpointer backend) {
  HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
  const HParser *p = h_permutation(h_ch('a'), h_ch('b'), h_ch('c'), NULL);

  g_check_parse_match(p, be, "abc", 3, "(u0x61 u0x62 u0x63)");
  g_check_parse_match(p, be, "acb", 3, "(u0x61 u0x62 u0x63)");
  g_check_parse_match(p, be, "bac", 3, "(u0x61 u0x62 u0x63)");
  g_check_parse_match(p, be, "bca", 3, "(u0x61 u0x62 u0x63)");
  g_check_parse_match(p, be, "cab", 3, "(u0x61 u0x62 u0x63)");
  g_check_parse_match(p, be, "cba", 3, "(u0x61 u0x62 u0x63)");
  g_check_parse_failed(p, be, "a", 1);
  g_check_parse_failed(p, be, "ab", 2);
  g_check_parse_failed(p, be, "abb", 3);

  const HParser *po = h_permutation(h_ch('a'), h_ch('b'), h_optional(h_ch('c')), NULL);

  g_check_parse_match(po, be, "abc", 3, "(u0x61 u0x62 u0x63)");
  g_check_parse_match(po, be, "acb", 3, "(u0x61 u0x62 u0x63)");
  g_check_parse_match(po, be, "bac", 3, "(u0x61 u0x62 u0x63)");
  g_check_parse_match(po, be, "bca", 3, "(u0x61 u0x62 u0x63)");
  g_check_parse_match(po, be, "cab", 3, "(u0x61 u0x62 u0x63)");
  g_check_parse_match(po, be, "cba", 3, "(u0x61 u0x62 u0x63)");
  g_check_parse_match(po, be, "ab", 2, "(u0x61 u0x62 null)");
  g_check_parse_match(po, be, "ba", 2, "(u0x61 u0x62 null)");
  g_check_parse_failed(po, be, "a", 1);
  g_check_parse_failed(po, be, "b", 1);
  g_check_parse_failed(po, be, "c", 1);
  g_check_parse_failed(po, be, "ca", 2);
  g_check_parse_failed(po, be, "cb", 2);
  g_check_parse_failed(po, be, "cc", 2);
  g_check_parse_failed(po, be, "ccab", 4);
  g_check_parse_failed(po, be, "ccc", 3);

  const HParser *po2 = h_permutation(h_optional(h_ch('c')), h_ch('a'), h_ch('b'), NULL);

  g_check_parse_match(po2, be, "abc", 3, "(u0x63 u0x61 u0x62)");
  g_check_parse_match(po2, be, "acb", 3, "(u0x63 u0x61 u0x62)");
  g_check_parse_match(po2, be, "bac", 3, "(u0x63 u0x61 u0x62)");
  g_check_parse_match(po2, be, "bca", 3, "(u0x63 u0x61 u0x62)");
  g_check_parse_match(po2, be, "cab", 3, "(u0x63 u0x61 u0x62)");
  g_check_parse_match(po2, be, "cba", 3, "(u0x63 u0x61 u0x62)");
  g_check_parse_match(po2, be, "ab", 2, "(null u0x61 u0x62)");
  g_check_parse_match(po2, be, "ba", 2, "(null u0x61 u0x62)");
  g_check_parse_failed(po2, be, "a", 1);
  g_check_parse_failed(po2, be, "b", 1);
  g_check_parse_failed(po2, be, "c", 1);
  g_check_parse_failed(po2, be, "ca", 2);
  g_check_parse_failed(po2, be, "cb", 2);
  g_check_parse_failed(po2, be, "cc", 2);
  g_check_parse_failed(po2, be, "ccab", 4);
  g_check_parse_failed(po2, be, "ccc", 3);
}

static HParser *k_test_bind(HAllocator *mm__, const HParsedToken *p, void *env) {
  uint8_t one = (uintptr_t)env;
  
  assert(p);
  assert(p->token_type == TT_SEQUENCE);

  int v=0;
  for(size_t i=0; i<p->seq->used; i++) {
    assert(p->seq->elements[i]->token_type == TT_UINT);
    v = v*10 + p->seq->elements[i]->uint - '0';
  }

  if(v > 26)
    return h_nothing_p__m(mm__);	// fail
  else if(v > 127)
    return NULL;			// equivalent to the above
  else
    return h_ch__m(mm__, one - 1 + v);
}
static void test_bind(gconstpointer backend) {
  HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
  const HParser *digit = h_ch_range('0', '9');
  const HParser *nat = h_many1(digit);
  const HParser *p = h_bind(nat, k_test_bind, (void *)(uintptr_t)'a');

  g_check_parse_match(p, be, "1a", 2, "u0x61");
  g_check_parse_match(p, be, "2b", 2, "u0x62");
  g_check_parse_match(p, be, "26z", 3, "u0x7a");
  g_check_parse_failed(p, be, "1x", 2);
  g_check_parse_failed(p, be, "29y", 3);
  g_check_parse_failed(p, be, "@", 1);
  g_check_parse_failed(p, be, "27{", 3);
  g_check_parse_failed(p, be, "272{", 4);
}

static void test_skip(gconstpointer backend) {
  HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
  const HParser *p, *p_le, *p_be;

  p = h_sequence(h_ch('a'), h_skip(32), h_ch('f'), NULL);
  g_check_parse_match(p, be, "abcdef", 6, "(u0x61 u0x66)");
  g_check_parse_failed(p, be, "abcdex", 6);
  g_check_parse_failed(p, be, "abc", 3);

  p = h_sequence(h_ch('a'), h_skip(32), h_end_p(), NULL);
  g_check_parse_match(p, be, "abcde", 5, "(u0x61)");

  p = h_sequence(h_ch('a'), h_skip(3), h_ch('\0'), h_skip(5), h_ch('b'), NULL);
  g_check_parse_match(p, be, "a\xe0\x1f\x62", 4, "(u0x61 u0 u0x62)"); // big-endian
  p_le = h_with_endianness(BYTE_LITTLE_ENDIAN|BIT_LITTLE_ENDIAN, p);
  p_be = h_with_endianness(BYTE_LITTLE_ENDIAN|BIT_BIG_ENDIAN, p);
  g_check_parse_match(p_be, be, "a\xe0\x1f\x62", 4, "(u0x61 u0 u0x62)");
  g_check_parse_match(p_le, be, "a\x07\xf8\x62", 4, "(u0x61 u0 u0x62)");

  p = h_sequence(h_ch('a'), h_skip(3), h_ch('\0'), h_skip(5), h_end_p(), NULL);
  g_check_parse_match(p, be, "a\xe0\x1f", 3, "(u0x61 u0)"); // big-endian
}

static void test_tell(gconstpointer backend) {
  HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
  const HParser *p;

  p = h_sequence(h_ch('a'), h_ch('b'), h_tell(), h_end_p(), NULL);
  g_check_parse_match(p, be, "ab", 2, "(u0x61 u0x62 u0x10)");
  g_check_parse_failed(p, be, "abc", 1);
  g_check_parse_failed(p, be, "a", 1);
}

static void test_seek(gconstpointer backend) {
  HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
  const HParser *p;

  p = h_sequence(h_ch('a'), h_seek(40, SEEK_SET), h_ch('f'), NULL);
  g_check_parse_match(p, be, "abcdef", 6, "(u0x61 u0x28 u0x66)");
  g_check_parse_failed(p, be, "abcdex", 6);
  g_check_parse_failed(p, be, "abc", 3);

  p = h_sequence(h_ch('a'), h_seek(40, SEEK_SET), h_end_p(), NULL);
  g_check_parse_match(p, be, "abcde", 5, "(u0x61 u0x28)");
  g_check_parse_failed(p, be, "abcdex", 6);
  g_check_parse_failed(p, be, "abc", 3);

  p = h_sequence(h_ch('a'), h_seek(0, SEEK_END), h_end_p(), NULL);
  g_check_parse_match(p, be, "abcde", 5, "(u0x61 u0x28)");
  g_check_parse_match(p, be, "abc", 3, "(u0x61 u0x18)");

  p = h_sequence(h_ch('a'), h_seek(-16, SEEK_END), h_ch('x'), NULL);
  g_check_parse_match(p, be, "abcdxy", 6, "(u0x61 u0x20 u0x78)");
  g_check_parse_match(p, be, "abxy", 4, "(u0x61 u0x10 u0x78)");
  g_check_parse_failed(p, be, "abc", 3);
  g_check_parse_failed(p, be, "x", 1);

  p = h_sequence(h_ch('a'), h_seek(32, SEEK_CUR), h_ch('f'), NULL);
  g_check_parse_match(p, be, "abcdef", 6, "(u0x61 u0x28 u0x66)");
  g_check_parse_failed(p, be, "xbcdef", 6);
  g_check_parse_failed(p, be, "abcdex", 6);
  g_check_parse_failed(p, be, "abc", 3);
}

void register_parser_tests(void) {
  g_test_add_data_func("/core/parser/packrat/token", GINT_TO_POINTER(PB_PACKRAT), test_token);
  g_test_add_data_func("/core/parser/packrat/ch", GINT_TO_POINTER(PB_PACKRAT), test_ch);
  g_test_add_data_func("/core/parser/packrat/ch_range", GINT_TO_POINTER(PB_PACKRAT), test_ch_range);
  g_test_add_data_func("/core/parser/packrat/int64", GINT_TO_POINTER(PB_PACKRAT), test_int64);
  g_test_add_data_func("/core/parser/packrat/int32", GINT_TO_POINTER(PB_PACKRAT), test_int32);
  g_test_add_data_func("/core/parser/packrat/int16", GINT_TO_POINTER(PB_PACKRAT), test_int16);
  g_test_add_data_func("/core/parser/packrat/int8", GINT_TO_POINTER(PB_PACKRAT), test_int8);
  g_test_add_data_func("/core/parser/packrat/uint64", GINT_TO_POINTER(PB_PACKRAT), test_uint64);
  g_test_add_data_func("/core/parser/packrat/uint32", GINT_TO_POINTER(PB_PACKRAT), test_uint32);
  g_test_add_data_func("/core/parser/packrat/uint16", GINT_TO_POINTER(PB_PACKRAT), test_uint16);
  g_test_add_data_func("/core/parser/packrat/uint8", GINT_TO_POINTER(PB_PACKRAT), test_uint8);
  g_test_add_data_func("/core/parser/packrat/int_range", GINT_TO_POINTER(PB_PACKRAT), test_int_range);
#if 0
  g_test_add_data_func("/core/parser/packrat/float64", GINT_TO_POINTER(PB_PACKRAT), test_float64);
  g_test_add_data_func("/core/parser/packrat/float32", GINT_TO_POINTER(PB_PACKRAT), test_float32);
#endif
  g_test_add_data_func("/core/parser/packrat/whitespace", GINT_TO_POINTER(PB_PACKRAT), test_whitespace);
  g_test_add_data_func("/core/parser/packrat/left", GINT_TO_POINTER(PB_PACKRAT), test_left);
  g_test_add_data_func("/core/parser/packrat/right", GINT_TO_POINTER(PB_PACKRAT), test_right);
  g_test_add_data_func("/core/parser/packrat/middle", GINT_TO_POINTER(PB_PACKRAT), test_middle);
  g_test_add_data_func("/core/parser/packrat/action", GINT_TO_POINTER(PB_PACKRAT), test_action);
  g_test_add_data_func("/core/parser/packrat/in", GINT_TO_POINTER(PB_PACKRAT), test_in);
  g_test_add_data_func("/core/parser/packrat/not_in", GINT_TO_POINTER(PB_PACKRAT), test_not_in);
  g_test_add_data_func("/core/parser/packrat/end_p", GINT_TO_POINTER(PB_PACKRAT), test_end_p);
  g_test_add_data_func("/core/parser/packrat/nothing_p", GINT_TO_POINTER(PB_PACKRAT), test_nothing_p);
  g_test_add_data_func("/core/parser/packrat/sequence", GINT_TO_POINTER(PB_PACKRAT), test_sequence);
  g_test_add_data_func("/core/parser/packrat/choice", GINT_TO_POINTER(PB_PACKRAT), test_choice);
  g_test_add_data_func("/core/parser/packrat/butnot", GINT_TO_POINTER(PB_PACKRAT), test_butnot);
  g_test_add_data_func("/core/parser/packrat/difference", GINT_TO_POINTER(PB_PACKRAT), test_difference);
  g_test_add_data_func("/core/parser/packrat/xor", GINT_TO_POINTER(PB_PACKRAT), test_xor);
  g_test_add_data_func("/core/parser/packrat/many", GINT_TO_POINTER(PB_PACKRAT), test_many);
  g_test_add_data_func("/core/parser/packrat/many1", GINT_TO_POINTER(PB_PACKRAT), test_many1);
  g_test_add_data_func("/core/parser/packrat/repeat_n", GINT_TO_POINTER(PB_PACKRAT), test_repeat_n);
  g_test_add_data_func("/core/parser/packrat/optional", GINT_TO_POINTER(PB_PACKRAT), test_optional);
  g_test_add_data_func("/core/parser/packrat/sepBy", GINT_TO_POINTER(PB_PACKRAT), test_sepBy);
  g_test_add_data_func("/core/parser/packrat/sepBy1", GINT_TO_POINTER(PB_PACKRAT), test_sepBy1);
  g_test_add_data_func("/core/parser/packrat/epsilon_p", GINT_TO_POINTER(PB_PACKRAT), test_epsilon_p);
  g_test_add_data_func("/core/parser/packrat/attr_bool", GINT_TO_POINTER(PB_PACKRAT), test_attr_bool);
  g_test_add_data_func("/core/parser/packrat/and", GINT_TO_POINTER(PB_PACKRAT), test_and);
  g_test_add_data_func("/core/parser/packrat/not", GINT_TO_POINTER(PB_PACKRAT), test_not);
  g_test_add_data_func("/core/parser/packrat/ignore", GINT_TO_POINTER(PB_PACKRAT), test_ignore);
  // XXX(pesco) it seems to me Warth's algorithm just doesn't work for this case
  //g_test_add_data_func("/core/parser/packrat/leftrec", GINT_TO_POINTER(PB_PACKRAT), test_leftrec);
  g_test_add_data_func("/core/parser/packrat/leftrec-ne", GINT_TO_POINTER(PB_PACKRAT), test_leftrec_ne);
  g_test_add_data_func("/core/parser/packrat/rightrec", GINT_TO_POINTER(PB_PACKRAT), test_rightrec);
  g_test_add_data_func("/core/parser/packrat/endianness", GINT_TO_POINTER(PB_PACKRAT), test_endianness);
  g_test_add_data_func("/core/parser/packrat/putget", GINT_TO_POINTER(PB_PACKRAT), test_put_get);
  g_test_add_data_func("/core/parser/packrat/permutation", GINT_TO_POINTER(PB_PACKRAT), test_permutation);
  g_test_add_data_func("/core/parser/packrat/bind", GINT_TO_POINTER(PB_PACKRAT), test_bind);
  g_test_add_data_func("/core/parser/packrat/result_length", GINT_TO_POINTER(PB_PACKRAT), test_result_length);
  //g_test_add_data_func("/core/parser/packrat/token_position", GINT_TO_POINTER(PB_PACKRAT), test_token_position);
  g_test_add_data_func("/core/parser/packrat/skip", GINT_TO_POINTER(PB_PACKRAT), test_skip);
  g_test_add_data_func("/core/parser/packrat/seek", GINT_TO_POINTER(PB_PACKRAT), test_seek);
  g_test_add_data_func("/core/parser/packrat/tell", GINT_TO_POINTER(PB_PACKRAT), test_tell);

  g_test_add_data_func("/core/parser/llk/token", GINT_TO_POINTER(PB_LLk), test_token);
  g_test_add_data_func("/core/parser/llk/ch", GINT_TO_POINTER(PB_LLk), test_ch);
  g_test_add_data_func("/core/parser/llk/ch_range", GINT_TO_POINTER(PB_LLk), test_ch_range);
  g_test_add_data_func("/core/parser/llk/int64", GINT_TO_POINTER(PB_LLk), test_int64);
  g_test_add_data_func("/core/parser/llk/int32", GINT_TO_POINTER(PB_LLk), test_int32);
  g_test_add_data_func("/core/parser/llk/int16", GINT_TO_POINTER(PB_LLk), test_int16);
  g_test_add_data_func("/core/parser/llk/int8", GINT_TO_POINTER(PB_LLk), test_int8);
  g_test_add_data_func("/core/parser/llk/uint64", GINT_TO_POINTER(PB_LLk), test_uint64);
  g_test_add_data_func("/core/parser/llk/uint32", GINT_TO_POINTER(PB_LLk), test_uint32);
  g_test_add_data_func("/core/parser/llk/uint16", GINT_TO_POINTER(PB_LLk), test_uint16);
  g_test_add_data_func("/core/parser/llk/uint8", GINT_TO_POINTER(PB_LLk), test_uint8);
  g_test_add_data_func("/core/parser/llk/int_range", GINT_TO_POINTER(PB_LLk), test_int_range);
#if 0
  g_test_add_data_func("/core/parser/llk/float64", GINT_TO_POINTER(PB_LLk), test_float64);
  g_test_add_data_func("/core/parser/llk/float32", GINT_TO_POINTER(PB_LLk), test_float32);
#endif
  g_test_add_data_func("/core/parser/llk/whitespace", GINT_TO_POINTER(PB_LLk), test_whitespace);
  g_test_add_data_func("/core/parser/llk/left", GINT_TO_POINTER(PB_LLk), test_left);
  g_test_add_data_func("/core/parser/llk/right", GINT_TO_POINTER(PB_LLk), test_right);
  g_test_add_data_func("/core/parser/llk/middle", GINT_TO_POINTER(PB_LLk), test_middle);
  g_test_add_data_func("/core/parser/llk/action", GINT_TO_POINTER(PB_LLk), test_action);
  g_test_add_data_func("/core/parser/llk/in", GINT_TO_POINTER(PB_LLk), test_in);
  g_test_add_data_func("/core/parser/llk/not_in", GINT_TO_POINTER(PB_LLk), test_not_in);
  g_test_add_data_func("/core/parser/llk/end_p", GINT_TO_POINTER(PB_LLk), test_end_p);
  g_test_add_data_func("/core/parser/llk/nothing_p", GINT_TO_POINTER(PB_LLk), test_nothing_p);
  g_test_add_data_func("/core/parser/llk/sequence", GINT_TO_POINTER(PB_LLk), test_sequence);
  g_test_add_data_func("/core/parser/llk/choice", GINT_TO_POINTER(PB_LLk), test_choice);
  g_test_add_data_func("/core/parser/llk/many", GINT_TO_POINTER(PB_LLk), test_many);
  g_test_add_data_func("/core/parser/llk/many1", GINT_TO_POINTER(PB_LLk), test_many1);
  g_test_add_data_func("/core/parser/llk/optional", GINT_TO_POINTER(PB_LLk), test_optional);
  g_test_add_data_func("/core/parser/llk/sepBy", GINT_TO_POINTER(PB_LLk), test_sepBy);
  g_test_add_data_func("/core/parser/llk/sepBy1", GINT_TO_POINTER(PB_LLk), test_sepBy1);
  g_test_add_data_func("/core/parser/llk/epsilon_p", GINT_TO_POINTER(PB_LLk), test_epsilon_p);
  g_test_add_data_func("/core/parser/llk/attr_bool", GINT_TO_POINTER(PB_LLk), test_attr_bool);
  g_test_add_data_func("/core/parser/llk/ignore", GINT_TO_POINTER(PB_LLk), test_ignore);
  //g_test_add_data_func("/core/parser/llk/leftrec", GINT_TO_POINTER(PB_LLk), test_leftrec);
  g_test_add_data_func("/core/parser/llk/rightrec", GINT_TO_POINTER(PB_LLk), test_rightrec);
 g_test_add_data_func("/core/parser/llk/result_length", GINT_TO_POINTER(PB_LLk), test_result_length);
  //g_test_add_data_func("/core/parser/llk/token_position", GINT_TO_POINTER(PB_LLk), test_token_position);
  g_test_add_data_func("/core/parser/llk/iterative", GINT_TO_POINTER(PB_LLk), test_iterative);
  g_test_add_data_func("/core/parser/llk/iterative/lookahead", GINT_TO_POINTER(PB_LLk), test_iterative_lookahead);
  g_test_add_data_func("/core/parser/llk/iterative/result_length", GINT_TO_POINTER(PB_LLk), test_iterative_result_length);

  g_test_add_data_func("/core/parser/regex/token", GINT_TO_POINTER(PB_REGULAR), test_token);
  g_test_add_data_func("/core/parser/regex/ch", GINT_TO_POINTER(PB_REGULAR), test_ch);
  g_test_add_data_func("/core/parser/regex/ch_range", GINT_TO_POINTER(PB_REGULAR), test_ch_range);
  g_test_add_data_func("/core/parser/regex/int64", GINT_TO_POINTER(PB_REGULAR), test_int64);
  g_test_add_data_func("/core/parser/regex/int32", GINT_TO_POINTER(PB_REGULAR), test_int32);
  g_test_add_data_func("/core/parser/regex/int16", GINT_TO_POINTER(PB_REGULAR), test_int16);
  g_test_add_data_func("/core/parser/regex/int8", GINT_TO_POINTER(PB_REGULAR), test_int8);
  g_test_add_data_func("/core/parser/regex/uint64", GINT_TO_POINTER(PB_REGULAR), test_uint64);
  g_test_add_data_func("/core/parser/regex/uint32", GINT_TO_POINTER(PB_REGULAR), test_uint32);
  g_test_add_data_func("/core/parser/regex/uint16", GINT_TO_POINTER(PB_REGULAR), test_uint16);
  g_test_add_data_func("/core/parser/regex/uint8", GINT_TO_POINTER(PB_REGULAR), test_uint8);
#if 0
  g_test_add_data_func("/core/parser/regex/int_range", GINT_TO_POINTER(PB_REGULAR), test_int_range);
  g_test_add_data_func("/core/parser/regex/float64", GINT_TO_POINTER(PB_REGULAR), test_float64);
  g_test_add_data_func("/core/parser/regex/float32", GINT_TO_POINTER(PB_REGULAR), test_float32);
#endif
  g_test_add_data_func("/core/parser/regex/whitespace", GINT_TO_POINTER(PB_REGULAR), test_whitespace);
  g_test_add_data_func("/core/parser/regex/left", GINT_TO_POINTER(PB_REGULAR), test_left);
  g_test_add_data_func("/core/parser/regex/right", GINT_TO_POINTER(PB_REGULAR), test_right);
  g_test_add_data_func("/core/parser/regex/middle", GINT_TO_POINTER(PB_REGULAR), test_middle);
  g_test_add_data_func("/core/parser/regex/action", GINT_TO_POINTER(PB_REGULAR), test_action);
  g_test_add_data_func("/core/parser/regex/in", GINT_TO_POINTER(PB_REGULAR), test_in);
  g_test_add_data_func("/core/parser/regex/not_in", GINT_TO_POINTER(PB_REGULAR), test_not_in);
  g_test_add_data_func("/core/parser/regex/end_p", GINT_TO_POINTER(PB_REGULAR), test_end_p);
  g_test_add_data_func("/core/parser/regex/nothing_p", GINT_TO_POINTER(PB_REGULAR), test_nothing_p);
  g_test_add_data_func("/core/parser/regex/sequence", GINT_TO_POINTER(PB_REGULAR), test_sequence);
  g_test_add_data_func("/core/parser/regex/choice", GINT_TO_POINTER(PB_REGULAR), test_choice);
  g_test_add_data_func("/core/parser/regex/many", GINT_TO_POINTER(PB_REGULAR), test_many);
  g_test_add_data_func("/core/parser/regex/many1", GINT_TO_POINTER(PB_REGULAR), test_many1);
  g_test_add_data_func("/core/parser/regex/repeat_n", GINT_TO_POINTER(PB_REGULAR), test_repeat_n);
  g_test_add_data_func("/core/parser/regex/optional", GINT_TO_POINTER(PB_REGULAR), test_optional);
  g_test_add_data_func("/core/parser/regex/sepBy", GINT_TO_POINTER(PB_REGULAR), test_sepBy);
  g_test_add_data_func("/core/parser/regex/sepBy1", GINT_TO_POINTER(PB_REGULAR), test_sepBy1);
  g_test_add_data_func("/core/parser/regex/epsilon_p", GINT_TO_POINTER(PB_REGULAR), test_epsilon_p);
  g_test_add_data_func("/core/parser/regex/attr_bool", GINT_TO_POINTER(PB_REGULAR), test_attr_bool);
  g_test_add_data_func("/core/parser/regex/ignore", GINT_TO_POINTER(PB_REGULAR), test_ignore);
  g_test_add_data_func("/core/parser/regex/result_length", GINT_TO_POINTER(PB_REGULAR), test_result_length);
  g_test_add_data_func("/core/parser/regex/token_position", GINT_TO_POINTER(PB_REGULAR), test_token_position);

  g_test_add_data_func("/core/parser/lalr/token", GINT_TO_POINTER(PB_LALR), test_token);
  g_test_add_data_func("/core/parser/lalr/ch", GINT_TO_POINTER(PB_LALR), test_ch);
  g_test_add_data_func("/core/parser/lalr/ch_range", GINT_TO_POINTER(PB_LALR), test_ch_range);
  g_test_add_data_func("/core/parser/lalr/int64", GINT_TO_POINTER(PB_LALR), test_int64);
  g_test_add_data_func("/core/parser/lalr/int32", GINT_TO_POINTER(PB_LALR), test_int32);
  g_test_add_data_func("/core/parser/lalr/int16", GINT_TO_POINTER(PB_LALR), test_int16);
  g_test_add_data_func("/core/parser/lalr/int8", GINT_TO_POINTER(PB_LALR), test_int8);
  g_test_add_data_func("/core/parser/lalr/uint64", GINT_TO_POINTER(PB_LALR), test_uint64);
  g_test_add_data_func("/core/parser/lalr/uint32", GINT_TO_POINTER(PB_LALR), test_uint32);
  g_test_add_data_func("/core/parser/lalr/uint16", GINT_TO_POINTER(PB_LALR), test_uint16);
  g_test_add_data_func("/core/parser/lalr/uint8", GINT_TO_POINTER(PB_LALR), test_uint8);
  g_test_add_data_func("/core/parser/lalr/int_range", GINT_TO_POINTER(PB_LALR), test_int_range);
#if 0
  g_test_add_data_func("/core/parser/lalr/float64", GINT_TO_POINTER(PB_LALR), test_float64);
  g_test_add_data_func("/core/parser/lalr/float32", GINT_TO_POINTER(PB_LALR), test_float32);
#endif
  g_test_add_data_func("/core/parser/lalr/whitespace", GINT_TO_POINTER(PB_LALR), test_whitespace);
  g_test_add_data_func("/core/parser/lalr/left", GINT_TO_POINTER(PB_LALR), test_left);
  g_test_add_data_func("/core/parser/lalr/right", GINT_TO_POINTER(PB_LALR), test_right);
  g_test_add_data_func("/core/parser/lalr/middle", GINT_TO_POINTER(PB_LALR), test_middle);
  g_test_add_data_func("/core/parser/lalr/action", GINT_TO_POINTER(PB_LALR), test_action);
  g_test_add_data_func("/core/parser/lalr/in", GINT_TO_POINTER(PB_LALR), test_in);
  g_test_add_data_func("/core/parser/lalr/not_in", GINT_TO_POINTER(PB_LALR), test_not_in);
  g_test_add_data_func("/core/parser/lalr/end_p", GINT_TO_POINTER(PB_LALR), test_end_p);
  g_test_add_data_func("/core/parser/lalr/nothing_p", GINT_TO_POINTER(PB_LALR), test_nothing_p);
  g_test_add_data_func("/core/parser/lalr/sequence", GINT_TO_POINTER(PB_LALR), test_sequence);
  g_test_add_data_func("/core/parser/lalr/choice", GINT_TO_POINTER(PB_LALR), test_choice);
  g_test_add_data_func("/core/parser/lalr/many", GINT_TO_POINTER(PB_LALR), test_many);
  g_test_add_data_func("/core/parser/lalr/many1", GINT_TO_POINTER(PB_LALR), test_many1);
  g_test_add_data_func("/core/parser/lalr/optional", GINT_TO_POINTER(PB_LALR), test_optional);
  g_test_add_data_func("/core/parser/lalr/sepBy", GINT_TO_POINTER(PB_LALR), test_sepBy);
  g_test_add_data_func("/core/parser/lalr/sepBy1", GINT_TO_POINTER(PB_LALR), test_sepBy1);
  g_test_add_data_func("/core/parser/lalr/epsilon_p", GINT_TO_POINTER(PB_LALR), test_epsilon_p);
  g_test_add_data_func("/core/parser/lalr/attr_bool", GINT_TO_POINTER(PB_LALR), test_attr_bool);
  g_test_add_data_func("/core/parser/lalr/ignore", GINT_TO_POINTER(PB_LALR), test_ignore);
  g_test_add_data_func("/core/parser/lalr/leftrec", GINT_TO_POINTER(PB_LALR), test_leftrec);
  g_test_add_data_func("/core/parser/lalr/leftrec-ne", GINT_TO_POINTER(PB_LALR), test_leftrec_ne);
  g_test_add_data_func("/core/parser/lalr/rightrec", GINT_TO_POINTER(PB_LALR), test_rightrec);
  g_test_add_data_func("/core/parser/lalr/result_length", GINT_TO_POINTER(PB_LALR), test_result_length);
  g_test_add_data_func("/core/parser/lalr/token_position", GINT_TO_POINTER(PB_LALR), test_token_position);
  g_test_add_data_func("/core/parser/lalr/iterative", GINT_TO_POINTER(PB_LALR), test_iterative);
  g_test_add_data_func("/core/parser/lalr/iterative/lookahead", GINT_TO_POINTER(PB_LALR), test_iterative_lookahead);
  g_test_add_data_func("/core/parser/lalr/iterative/result_length", GINT_TO_POINTER(PB_LALR), test_iterative_result_length);

  g_test_add_data_func("/core/parser/glr/token", GINT_TO_POINTER(PB_GLR), test_token);
  g_test_add_data_func("/core/parser/glr/ch", GINT_TO_POINTER(PB_GLR), test_ch);
  g_test_add_data_func("/core/parser/glr/ch_range", GINT_TO_POINTER(PB_GLR), test_ch_range);
  g_test_add_data_func("/core/parser/glr/int64", GINT_TO_POINTER(PB_GLR), test_int64);
  g_test_add_data_func("/core/parser/glr/int32", GINT_TO_POINTER(PB_GLR), test_int32);
  g_test_add_data_func("/core/parser/glr/int16", GINT_TO_POINTER(PB_GLR), test_int16);
  g_test_add_data_func("/core/parser/glr/int8", GINT_TO_POINTER(PB_GLR), test_int8);
  g_test_add_data_func("/core/parser/glr/uint64", GINT_TO_POINTER(PB_GLR), test_uint64);
  g_test_add_data_func("/core/parser/glr/uint32", GINT_TO_POINTER(PB_GLR), test_uint32);
  g_test_add_data_func("/core/parser/glr/uint16", GINT_TO_POINTER(PB_GLR), test_uint16);
  g_test_add_data_func("/core/parser/glr/uint8", GINT_TO_POINTER(PB_GLR), test_uint8);
  g_test_add_data_func("/core/parser/glr/int_range", GINT_TO_POINTER(PB_GLR), test_int_range);
#if 0
  g_test_add_data_func("/core/parser/glr/float64", GINT_TO_POINTER(PB_GLR), test_float64);
  g_test_add_data_func("/core/parser/glr/float32", GINT_TO_POINTER(PB_GLR), test_float32);
#endif
  g_test_add_data_func("/core/parser/glr/whitespace", GINT_TO_POINTER(PB_GLR), test_whitespace);
  g_test_add_data_func("/core/parser/glr/left", GINT_TO_POINTER(PB_GLR), test_left);
  g_test_add_data_func("/core/parser/glr/right", GINT_TO_POINTER(PB_GLR), test_right);
  g_test_add_data_func("/core/parser/glr/middle", GINT_TO_POINTER(PB_GLR), test_middle);
  g_test_add_data_func("/core/parser/glr/action", GINT_TO_POINTER(PB_GLR), test_action);
  g_test_add_data_func("/core/parser/glr/in", GINT_TO_POINTER(PB_GLR), test_in);
  g_test_add_data_func("/core/parser/glr/not_in", GINT_TO_POINTER(PB_GLR), test_not_in);
  g_test_add_data_func("/core/parser/glr/end_p", GINT_TO_POINTER(PB_GLR), test_end_p);
  g_test_add_data_func("/core/parser/glr/nothing_p", GINT_TO_POINTER(PB_GLR), test_nothing_p);
  g_test_add_data_func("/core/parser/glr/sequence", GINT_TO_POINTER(PB_GLR), test_sequence);
  g_test_add_data_func("/core/parser/glr/choice", GINT_TO_POINTER(PB_GLR), test_choice);
  g_test_add_data_func("/core/parser/glr/many", GINT_TO_POINTER(PB_GLR), test_many);
  g_test_add_data_func("/core/parser/glr/many1", GINT_TO_POINTER(PB_GLR), test_many1);
  g_test_add_data_func("/core/parser/glr/optional", GINT_TO_POINTER(PB_GLR), test_optional);
  g_test_add_data_func("/core/parser/glr/sepBy", GINT_TO_POINTER(PB_GLR), test_sepBy);
  g_test_add_data_func("/core/parser/glr/sepBy1", GINT_TO_POINTER(PB_GLR), test_sepBy1);
  g_test_add_data_func("/core/parser/glr/epsilon_p", GINT_TO_POINTER(PB_GLR), test_epsilon_p);
  g_test_add_data_func("/core/parser/glr/attr_bool", GINT_TO_POINTER(PB_GLR), test_attr_bool);
  g_test_add_data_func("/core/parser/glr/ignore", GINT_TO_POINTER(PB_GLR), test_ignore);
  g_test_add_data_func("/core/parser/glr/leftrec", GINT_TO_POINTER(PB_GLR), test_leftrec);
  g_test_add_data_func("/core/parser/glr/leftrec-ne", GINT_TO_POINTER(PB_GLR), test_leftrec_ne);
  g_test_add_data_func("/core/parser/glr/rightrec", GINT_TO_POINTER(PB_GLR), test_rightrec);
  g_test_add_data_func("/core/parser/glr/ambiguous", GINT_TO_POINTER(PB_GLR), test_ambiguous);
  g_test_add_data_func("/core/parser/glr/result_length", GINT_TO_POINTER(PB_GLR), test_result_length);
  g_test_add_data_func("/core/parser/glr/token_position", GINT_TO_POINTER(PB_GLR), test_token_position);
}
