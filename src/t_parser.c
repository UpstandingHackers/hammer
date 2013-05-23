#include <glib.h>
#include <string.h>
#include "hammer.h"
#include "internal.h"
#include "test_suite.h"
#include "parsers/parser_internal.h"

static void test_token(gconstpointer backend) {
  const HParser *token_ = h_token((const uint8_t*)"95\xa2", 3);

  g_check_parse_ok(token_, (HParserBackend)GPOINTER_TO_INT(backend), "95\xa2", 3, "<39.35.a2>");
  g_check_parse_failed(token_, (HParserBackend)GPOINTER_TO_INT(backend), "95", 2);
}

static void test_ch(gconstpointer backend) {
  const HParser *ch_ = h_ch(0xa2);

  g_check_parse_ok(ch_, (HParserBackend)GPOINTER_TO_INT(backend), "\xa2", 1, "u0xa2");
  g_check_parse_failed(ch_, (HParserBackend)GPOINTER_TO_INT(backend), "\xa3", 1);
}

static void test_ch_range(gconstpointer backend) {
  const HParser *range_ = h_ch_range('a', 'c');

  g_check_parse_ok(range_, (HParserBackend)GPOINTER_TO_INT(backend), "b", 1, "u0x62");
  g_check_parse_failed(range_, (HParserBackend)GPOINTER_TO_INT(backend), "d", 1);
}

//@MARK_START
static void test_int64(gconstpointer backend) {
  const HParser *int64_ = h_int64();

  g_check_parse_ok(int64_, (HParserBackend)GPOINTER_TO_INT(backend), "\xff\xff\xff\xfe\x00\x00\x00\x00", 8, "s-0x200000000");
  g_check_parse_failed(int64_, (HParserBackend)GPOINTER_TO_INT(backend), "\xff\xff\xff\xfe\x00\x00\x00", 7);
}

static void test_int32(gconstpointer backend) {
  const HParser *int32_ = h_int32();

  g_check_parse_ok(int32_, (HParserBackend)GPOINTER_TO_INT(backend), "\xff\xfe\x00\x00", 4, "s-0x20000");
  g_check_parse_failed(int32_, (HParserBackend)GPOINTER_TO_INT(backend), "\xff\xfe\x00", 3);

  g_check_parse_ok(int32_, (HParserBackend)GPOINTER_TO_INT(backend), "\x00\x02\x00\x00", 4, "s0x20000");
  g_check_parse_failed(int32_, (HParserBackend)GPOINTER_TO_INT(backend), "\x00\x02\x00", 3);
}

static void test_int16(gconstpointer backend) {
  const HParser *int16_ = h_int16();

  g_check_parse_ok(int16_, (HParserBackend)GPOINTER_TO_INT(backend), "\xfe\x00", 2, "s-0x200");
  g_check_parse_failed(int16_, (HParserBackend)GPOINTER_TO_INT(backend), "\xfe", 1);

  g_check_parse_ok(int16_, (HParserBackend)GPOINTER_TO_INT(backend), "\x02\x00", 2, "s0x200");
  g_check_parse_failed(int16_, (HParserBackend)GPOINTER_TO_INT(backend), "\x02", 1);
}

static void test_int8(gconstpointer backend) {
  const HParser *int8_ = h_int8();

  g_check_parse_ok(int8_, (HParserBackend)GPOINTER_TO_INT(backend), "\x88", 1, "s-0x78");
  g_check_parse_failed(int8_, (HParserBackend)GPOINTER_TO_INT(backend), "", 0);
}

static void test_uint64(gconstpointer backend) {
  const HParser *uint64_ = h_uint64();

  g_check_parse_ok(uint64_, (HParserBackend)GPOINTER_TO_INT(backend), "\x00\x00\x00\x02\x00\x00\x00\x00", 8, "u0x200000000");
  g_check_parse_failed(uint64_, (HParserBackend)GPOINTER_TO_INT(backend), "\x00\x00\x00\x02\x00\x00\x00", 7);
}

static void test_uint32(gconstpointer backend) {
  const HParser *uint32_ = h_uint32();

  g_check_parse_ok(uint32_, (HParserBackend)GPOINTER_TO_INT(backend), "\x00\x02\x00\x00", 4, "u0x20000");
  g_check_parse_failed(uint32_, (HParserBackend)GPOINTER_TO_INT(backend), "\x00\x02\x00", 3);
}

static void test_uint16(gconstpointer backend) {
  const HParser *uint16_ = h_uint16();

  g_check_parse_ok(uint16_, (HParserBackend)GPOINTER_TO_INT(backend), "\x02\x00", 2, "u0x200");
  g_check_parse_failed(uint16_, (HParserBackend)GPOINTER_TO_INT(backend), "\x02", 1);
}

static void test_uint8(gconstpointer backend) {
  const HParser *uint8_ = h_uint8();

  g_check_parse_ok(uint8_, (HParserBackend)GPOINTER_TO_INT(backend), "\x78", 1, "u0x78");
  g_check_parse_failed(uint8_, (HParserBackend)GPOINTER_TO_INT(backend), "", 0);
}
//@MARK_END

static void test_int_range(gconstpointer backend) {
  const HParser *int_range_ = h_int_range(h_uint8(), 3, 10);
  
  g_check_parse_ok(int_range_, (HParserBackend)GPOINTER_TO_INT(backend), "\x05", 1, "u0x5");
  g_check_parse_failed(int_range_, (HParserBackend)GPOINTER_TO_INT(backend), "\xb", 1);
}

#if 0
static void test_float64(gconstpointer backend) {
  const HParser *float64_ = h_float64();

  g_check_parse_ok(float64_, (HParserBackend)GPOINTER_TO_INT(backend), "\x3f\xf0\x00\x00\x00\x00\x00\x00", 8, 1.0);
  g_check_parse_failed(float64_, (HParserBackend)GPOINTER_TO_INT(backend), "\x3f\xf0\x00\x00\x00\x00\x00", 7);
}

static void test_float32(gconstpointer backend) {
  const HParser *float32_ = h_float32();

  g_check_parse_ok(float32_, (HParserBackend)GPOINTER_TO_INT(backend), "\x3f\x80\x00\x00", 4, 1.0);
  g_check_parse_failed(float32_, (HParserBackend)GPOINTER_TO_INT(backend), "\x3f\x80\x00");
}
#endif


static void test_whitespace(gconstpointer backend) {
  const HParser *whitespace_ = h_whitespace(h_ch('a'));
  const HParser *whitespace_end = h_whitespace(h_end_p());

  g_check_parse_ok(whitespace_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1, "u0x61");
  g_check_parse_ok(whitespace_, (HParserBackend)GPOINTER_TO_INT(backend), " a", 2, "u0x61");
  g_check_parse_ok(whitespace_, (HParserBackend)GPOINTER_TO_INT(backend), "  a", 3, "u0x61");
  g_check_parse_ok(whitespace_, (HParserBackend)GPOINTER_TO_INT(backend), "\ta", 2, "u0x61");
  g_check_parse_failed(whitespace_, (HParserBackend)GPOINTER_TO_INT(backend), "_a", 2);

  g_check_parse_ok(whitespace_end, (HParserBackend)GPOINTER_TO_INT(backend), "", 0, "NULL");
  g_check_parse_ok(whitespace_end, (HParserBackend)GPOINTER_TO_INT(backend),"  ", 2, "NULL");
  g_check_parse_failed(whitespace_end, (HParserBackend)GPOINTER_TO_INT(backend),"  x", 3);
}

static void test_left(gconstpointer backend) {
  const HParser *left_ = h_left(h_ch('a'), h_ch(' '));

  g_check_parse_ok(left_, (HParserBackend)GPOINTER_TO_INT(backend), "a ", 2, "u0x61");
  g_check_parse_failed(left_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1);
  g_check_parse_failed(left_, (HParserBackend)GPOINTER_TO_INT(backend), " ", 1);
  g_check_parse_failed(left_, (HParserBackend)GPOINTER_TO_INT(backend), "ab", 2);
}

static void test_right(gconstpointer backend) {
  const HParser *right_ = h_right(h_ch(' '), h_ch('a'));

  g_check_parse_ok(right_, (HParserBackend)GPOINTER_TO_INT(backend), " a", 2, "u0x61");
  g_check_parse_failed(right_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1);
  g_check_parse_failed(right_, (HParserBackend)GPOINTER_TO_INT(backend), " ", 1);
  g_check_parse_failed(right_, (HParserBackend)GPOINTER_TO_INT(backend), "ba", 2);
}

static void test_middle(gconstpointer backend) {
  const HParser *middle_ = h_middle(h_ch(' '), h_ch('a'), h_ch(' '));

  g_check_parse_ok(middle_, (HParserBackend)GPOINTER_TO_INT(backend), " a ", 3, "u0x61");
  g_check_parse_failed(middle_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1);
  g_check_parse_failed(middle_, (HParserBackend)GPOINTER_TO_INT(backend), " ", 1);
  g_check_parse_failed(middle_, (HParserBackend)GPOINTER_TO_INT(backend), " a", 2);
  g_check_parse_failed(middle_, (HParserBackend)GPOINTER_TO_INT(backend), "a ", 2);
  g_check_parse_failed(middle_, (HParserBackend)GPOINTER_TO_INT(backend), " b ", 3);
  g_check_parse_failed(middle_, (HParserBackend)GPOINTER_TO_INT(backend), "ba ", 3);
  g_check_parse_failed(middle_, (HParserBackend)GPOINTER_TO_INT(backend), " ab", 3);
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

static void test_action(gconstpointer backend) {
  const HParser *action_ = h_action(h_sequence(h_choice(h_ch('a'), 
							h_ch('A'), 
							NULL), 
					       h_choice(h_ch('b'), 
							h_ch('B'), 
						      NULL), 
					       NULL), 
				    upcase);
  
  g_check_parse_ok(action_, (HParserBackend)GPOINTER_TO_INT(backend), "ab", 2, "(u0x41 u0x42)");
  g_check_parse_ok(action_, (HParserBackend)GPOINTER_TO_INT(backend), "AB", 2, "(u0x41 u0x42)");
  g_check_parse_failed(action_, (HParserBackend)GPOINTER_TO_INT(backend), "XX", 2);
}

static void test_in(gconstpointer backend) {
  uint8_t options[3] = { 'a', 'b', 'c' };
  const HParser *in_ = h_in(options, 3);
  g_check_parse_ok(in_, (HParserBackend)GPOINTER_TO_INT(backend), "b", 1, "u0x62");
  g_check_parse_failed(in_, (HParserBackend)GPOINTER_TO_INT(backend), "d", 1);

}

static void test_not_in(gconstpointer backend) {
  uint8_t options[3] = { 'a', 'b', 'c' };
  const HParser *not_in_ = h_not_in(options, 3);
  g_check_parse_ok(not_in_, (HParserBackend)GPOINTER_TO_INT(backend), "d", 1, "u0x64");
  g_check_parse_failed(not_in_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1);

}

static void test_end_p(gconstpointer backend) {
  const HParser *end_p_ = h_sequence(h_ch('a'), h_end_p(), NULL);
  g_check_parse_ok(end_p_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1, "(u0x61)");
  g_check_parse_failed(end_p_, (HParserBackend)GPOINTER_TO_INT(backend), "aa", 2);
}

static void test_nothing_p(gconstpointer backend) {
  const HParser *nothing_p_ = h_nothing_p();
  g_check_parse_failed(nothing_p_,  (HParserBackend)GPOINTER_TO_INT(backend),"a", 1);
}

static void test_sequence(gconstpointer backend) {
  const HParser *sequence_1 = h_sequence(h_ch('a'), h_ch('b'), NULL);
  const HParser *sequence_2 = h_sequence(h_ch('a'), h_whitespace(h_ch('b')), NULL);

  g_check_parse_ok(sequence_1, (HParserBackend)GPOINTER_TO_INT(backend), "ab", 2, "(u0x61 u0x62)");
  g_check_parse_failed(sequence_1, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1);
  g_check_parse_failed(sequence_1, (HParserBackend)GPOINTER_TO_INT(backend), "b", 1);
  g_check_parse_ok(sequence_2, (HParserBackend)GPOINTER_TO_INT(backend), "ab", 2, "(u0x61 u0x62)");
  g_check_parse_ok(sequence_2, (HParserBackend)GPOINTER_TO_INT(backend), "a b", 3, "(u0x61 u0x62)");
  g_check_parse_ok(sequence_2, (HParserBackend)GPOINTER_TO_INT(backend), "a  b", 4, "(u0x61 u0x62)");  
}

static void test_choice(gconstpointer backend) {
  const HParser *choice_ = h_choice(h_ch('a'), h_ch('b'), NULL);

  g_check_parse_ok(choice_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1, "u0x61");
  g_check_parse_ok(choice_, (HParserBackend)GPOINTER_TO_INT(backend), "b", 1, "u0x62");
  g_check_parse_failed(choice_, (HParserBackend)GPOINTER_TO_INT(backend), "c", 1);
}

static void test_butnot(gconstpointer backend) {
  const HParser *butnot_1 = h_butnot(h_ch('a'), h_token((const uint8_t*)"ab", 2));
  const HParser *butnot_2 = h_butnot(h_ch_range('0', '9'), h_ch('6'));

  g_check_parse_ok(butnot_1, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1, "u0x61");
  g_check_parse_failed(butnot_1, (HParserBackend)GPOINTER_TO_INT(backend), "ab", 2);
  g_check_parse_ok(butnot_1, (HParserBackend)GPOINTER_TO_INT(backend), "aa", 2, "u0x61");
  g_check_parse_failed(butnot_2, (HParserBackend)GPOINTER_TO_INT(backend), "6", 1);
}

static void test_difference(gconstpointer backend) {
  const HParser *difference_ = h_difference(h_token((const uint8_t*)"ab", 2), h_ch('a'));

  g_check_parse_ok(difference_, (HParserBackend)GPOINTER_TO_INT(backend), "ab", 2, "<61.62>");
  g_check_parse_failed(difference_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1);
}

static void test_xor(gconstpointer backend) {
  const HParser *xor_ = h_xor(h_ch_range('0', '6'), h_ch_range('5', '9'));

  g_check_parse_ok(xor_, (HParserBackend)GPOINTER_TO_INT(backend), "0", 1, "u0x30");
  g_check_parse_ok(xor_, (HParserBackend)GPOINTER_TO_INT(backend), "9", 1, "u0x39");
  g_check_parse_failed(xor_, (HParserBackend)GPOINTER_TO_INT(backend), "5", 1);
  g_check_parse_failed(xor_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1);
}

static void test_many(gconstpointer backend) {
  const HParser *many_ = h_many(h_choice(h_ch('a'), h_ch('b'), NULL));

  g_check_parse_ok(many_, (HParserBackend)GPOINTER_TO_INT(backend), "", 0, "()");
  g_check_parse_ok(many_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1, "(u0x61)");
  //  g_check_parse_ok(many_, (HParserBackend)GPOINTER_TO_INT(backend), "adef", 4, "(u0x61)");
  g_check_parse_ok(many_, (HParserBackend)GPOINTER_TO_INT(backend), "b", 1, "(u0x62)");
  //  g_check_parse_ok(many_, (HParserBackend)GPOINTER_TO_INT(backend), "bdef", 4, "(u0x62)");
  g_check_parse_ok(many_, (HParserBackend)GPOINTER_TO_INT(backend), "aabbaba", 7, "(u0x61 u0x61 u0x62 u0x62 u0x61 u0x62 u0x61)");
  //  g_check_parse_ok(many_, (HParserBackend)GPOINTER_TO_INT(backend), "aabbabadef", 10, "(u0x61 u0x61 u0x62 u0x62 u0x61 u0x62 u0x61)");
  //  g_check_parse_ok(many_, (HParserBackend)GPOINTER_TO_INT(backend), "daabbabadef", 11, "()");
}

static void test_many1(gconstpointer backend) {
  const HParser *many1_ = h_many1(h_choice(h_ch('a'), h_ch('b'), NULL));

  g_check_parse_failed(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "", 0);
  g_check_parse_ok(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1, "(u0x61)");
  //  g_check_parse_ok(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "adef", 4, "(u0x61)");
  g_check_parse_ok(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "b", 1, "(u0x62)");
  //  g_check_parse_ok(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "bdef", 4, "(u0x62)");
  g_check_parse_ok(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "aabbaba", 7, "(u0x61 u0x61 u0x62 u0x62 u0x61 u0x62 u0x61)");
  //  g_check_parse_ok(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "aabbabadef", 10, "(u0x61 u0x61 u0x62 u0x62 u0x61 u0x62 u0x61)");
  g_check_parse_failed(many1_, (HParserBackend)GPOINTER_TO_INT(backend), "daabbabadef", 11);  
}

static void test_repeat_n(gconstpointer backend) {
  const HParser *repeat_n_ = h_repeat_n(h_choice(h_ch('a'), h_ch('b'), NULL), 2);

  g_check_parse_failed(repeat_n_, (HParserBackend)GPOINTER_TO_INT(backend), "adef", 4);
  g_check_parse_ok(repeat_n_, (HParserBackend)GPOINTER_TO_INT(backend), "abdef", 5, "(u0x61 u0x62)");
  g_check_parse_failed(repeat_n_, (HParserBackend)GPOINTER_TO_INT(backend), "dabdef", 6);
}

static void test_optional(gconstpointer backend) {
  const HParser *optional_ = h_sequence(h_ch('a'), h_optional(h_choice(h_ch('b'), h_ch('c'), NULL)), h_ch('d'), NULL);
  
  g_check_parse_ok(optional_, (HParserBackend)GPOINTER_TO_INT(backend), "abd", 3, "(u0x61 u0x62 u0x64)");
  g_check_parse_ok(optional_, (HParserBackend)GPOINTER_TO_INT(backend), "acd", 3, "(u0x61 u0x63 u0x64)");
  g_check_parse_ok(optional_, (HParserBackend)GPOINTER_TO_INT(backend), "ad", 2, "(u0x61 null u0x64)");
  g_check_parse_failed(optional_, (HParserBackend)GPOINTER_TO_INT(backend), "aed", 3);
  g_check_parse_failed(optional_, (HParserBackend)GPOINTER_TO_INT(backend), "ab", 2);
  g_check_parse_failed(optional_, (HParserBackend)GPOINTER_TO_INT(backend), "ac", 2);
}

static void test_ignore(gconstpointer backend) {
  const HParser *ignore_ = h_sequence(h_ch('a'), h_ignore(h_ch('b')), h_ch('c'), NULL);

  g_check_parse_ok(ignore_, (HParserBackend)GPOINTER_TO_INT(backend), "abc", 3, "(u0x61 u0x63)");
  g_check_parse_failed(ignore_, (HParserBackend)GPOINTER_TO_INT(backend), "ac", 2);
}

static void test_sepBy(gconstpointer backend) {
  const HParser *sepBy_ = h_sepBy(h_choice(h_ch('1'), h_ch('2'), h_ch('3'), NULL), h_ch(','));

  g_check_parse_ok(sepBy_, (HParserBackend)GPOINTER_TO_INT(backend), "1,2,3", 5, "(u0x31 u0x32 u0x33)");
  g_check_parse_ok(sepBy_, (HParserBackend)GPOINTER_TO_INT(backend), "1,3,2", 5, "(u0x31 u0x33 u0x32)");
  g_check_parse_ok(sepBy_, (HParserBackend)GPOINTER_TO_INT(backend), "1,3", 3, "(u0x31 u0x33)");
  g_check_parse_ok(sepBy_, (HParserBackend)GPOINTER_TO_INT(backend), "3", 1, "(u0x33)");
  g_check_parse_ok(sepBy_, (HParserBackend)GPOINTER_TO_INT(backend), "", 0, "()");
}

static void test_sepBy1(gconstpointer backend) {
  const HParser *sepBy1_ = h_sepBy1(h_choice(h_ch('1'), h_ch('2'), h_ch('3'), NULL), h_ch(','));

  g_check_parse_ok(sepBy1_, (HParserBackend)GPOINTER_TO_INT(backend), "1,2,3", 5, "(u0x31 u0x32 u0x33)");
  g_check_parse_ok(sepBy1_, (HParserBackend)GPOINTER_TO_INT(backend), "1,3,2", 5, "(u0x31 u0x33 u0x32)");
  g_check_parse_ok(sepBy1_, (HParserBackend)GPOINTER_TO_INT(backend), "1,3", 3, "(u0x31 u0x33)");
  g_check_parse_ok(sepBy1_, (HParserBackend)GPOINTER_TO_INT(backend), "3", 1, "(u0x33)");
  g_check_parse_failed(sepBy1_, (HParserBackend)GPOINTER_TO_INT(backend), "", 0);
}

static void test_epsilon_p(gconstpointer backend) {
  const HParser *epsilon_p_1 = h_sequence(h_ch('a'), h_epsilon_p(), h_ch('b'), NULL);
  const HParser *epsilon_p_2 = h_sequence(h_epsilon_p(), h_ch('a'), NULL);
  const HParser *epsilon_p_3 = h_sequence(h_ch('a'), h_epsilon_p(), NULL);
  
  g_check_parse_ok(epsilon_p_1, (HParserBackend)GPOINTER_TO_INT(backend), "ab", 2, "(u0x61 u0x62)");
  g_check_parse_ok(epsilon_p_2, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1, "(u0x61)");
  g_check_parse_ok(epsilon_p_3, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1, "(u0x61)");
}

bool validate_test_ab(HParseResult *p) {
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
				   validate_test_ab);

  g_check_parse_ok(ab_, (HParserBackend)GPOINTER_TO_INT(backend), "aa", 2, "(u0x61 u0x61)");
  g_check_parse_ok(ab_, (HParserBackend)GPOINTER_TO_INT(backend), "bb", 2, "(u0x62 u0x62)");
  g_check_parse_failed(ab_, (HParserBackend)GPOINTER_TO_INT(backend), "ab", 2);
}

static void test_and(gconstpointer backend) {
  const HParser *and_1 = h_sequence(h_and(h_ch('0')), h_ch('0'), NULL);
  const HParser *and_2 = h_sequence(h_and(h_ch('0')), h_ch('1'), NULL);
  const HParser *and_3 = h_sequence(h_ch('1'), h_and(h_ch('2')), NULL);

  g_check_parse_ok(and_1, (HParserBackend)GPOINTER_TO_INT(backend), "0", 1, "(u0x30)");
  g_check_parse_failed(and_2, (HParserBackend)GPOINTER_TO_INT(backend), "0", 1);
  g_check_parse_ok(and_3, (HParserBackend)GPOINTER_TO_INT(backend), "12", 2, "(u0x31)");
}

static void test_not(gconstpointer backend) {
  const HParser *not_1 = h_sequence(h_ch('a'), h_choice(h_ch('+'), h_token((const uint8_t*)"++", 2), NULL), h_ch('b'), NULL);
  const HParser *not_2 = h_sequence(h_ch('a'),
				    h_choice(h_sequence(h_ch('+'), h_not(h_ch('+')), NULL),
					  h_token((const uint8_t*)"++", 2),
					  NULL), h_ch('b'), NULL);

  g_check_parse_ok(not_1, (HParserBackend)GPOINTER_TO_INT(backend), "a+b", 3, "(u0x61 u0x2b u0x62)");
  g_check_parse_failed(not_1, (HParserBackend)GPOINTER_TO_INT(backend), "a++b", 4);
  g_check_parse_ok(not_2, (HParserBackend)GPOINTER_TO_INT(backend), "a+b", 3, "(u0x61 (u0x2b) u0x62)");
  g_check_parse_ok(not_2, (HParserBackend)GPOINTER_TO_INT(backend), "a++b", 4, "(u0x61 <2b.2b> u0x62)");
}
/*
static void test_leftrec(gconstpointer backend) {
  const HParser *a_ = h_ch('a');

  HParser *lr_ = h_indirect();
  h_bind_indirect(lr_, h_choice(h_sequence(lr_, a_, NULL), a_, NULL));

  g_check_parse_ok(lr_, (HParserBackend)GPOINTER_TO_INT(backend), "a", 1, "u0x61");
  g_check_parse_ok(lr_, (HParserBackend)GPOINTER_TO_INT(backend), "aa", 2, "(u0x61 u0x61)");
  g_check_parse_ok(lr_, (HParserBackend)GPOINTER_TO_INT(backend), "aaa", 3, "((u0x61 u0x61) u0x61)");
}
*/
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
  //  g_test_add_data_func("/core/parser/packrat/leftrec", GINT_TO_POINTER(PB_PACKRAT), test_leftrec);

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
  g_test_add_data_func("/core/parser/regex/int_range", GINT_TO_POINTER(PB_REGULAR), test_int_range);
#if 0
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
  g_test_add_data_func("/core/parser/regex/optional", GINT_TO_POINTER(PB_REGULAR), test_optional);
  g_test_add_data_func("/core/parser/regex/sepBy", GINT_TO_POINTER(PB_REGULAR), test_sepBy);
  g_test_add_data_func("/core/parser/regex/sepBy1", GINT_TO_POINTER(PB_REGULAR), test_sepBy1);
  g_test_add_data_func("/core/parser/regex/epsilon_p", GINT_TO_POINTER(PB_REGULAR), test_epsilon_p);
  g_test_add_data_func("/core/parser/regex/attr_bool", GINT_TO_POINTER(PB_REGULAR), test_attr_bool);
  g_test_add_data_func("/core/parser/regex/ignore", GINT_TO_POINTER(PB_REGULAR), test_ignore);
}
