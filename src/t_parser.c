#include <glib.h>
#include <string.h>
#include "hammer.h"
#include "internal.h"
#include "test_suite.h"
#include "parsers/parser_internal.h"

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
  const HParser *whitespace_end = h_whitespace(h_end_p());

  g_check_parse_ok(whitespace_, "a", 1, "u0x61");
  g_check_parse_ok(whitespace_, " a", 2, "u0x61");
  g_check_parse_ok(whitespace_, "  a", 3, "u0x61");
  g_check_parse_ok(whitespace_, "\ta", 2, "u0x61");
  g_check_parse_failed(whitespace_, "_a", 2);

  g_check_parse_ok(whitespace_end, "", 0, "NULL");
  g_check_parse_ok(whitespace_end, "  ", 2, "NULL");
  g_check_parse_failed(whitespace_end, "  x", 3);
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

static void test_leftrec(void) {
  const HParser *a_ = h_ch('a');

  HParser *lr_ = h_indirect();
  h_bind_indirect(lr_, h_choice(h_sequence(lr_, a_, NULL), a_, NULL));

  g_check_parse_ok(lr_, "a", 1, "u0x61");
  g_check_parse_ok(lr_, "aa", 2, "(u0x61 u0x61)");
  g_check_parse_ok(lr_, "aaa", 3, "((u0x61 u0x61) u0x61)");
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
  g_test_add_func("/core/parser/leftrec", test_leftrec);
}
