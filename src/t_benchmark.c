#include <glib.h>
#include "hammer.h"
#include "test_suite.h"

HParserTestcase testcases[] = {
  {(unsigned char*)"1,2,3", 5, "(u0x31 u0x32 u0x33)"},
  {(unsigned char*)"1,3,2", 5, "(u0x31 u0x33 u0x32)"},
  {(unsigned char*)"1,3", 3, "(u0x31 u0x33)"},
  {(unsigned char*)"3", 1, "(u0x33)"},
  { NULL, 0, NULL }
};

static void test_benchmark_1() {
  HParser *parser = h_sepBy1(h_choice(h_ch('1'), h_ch('2'), h_ch('3'), NULL), h_ch(',')); 

  HBenchmarkResults *res = h_benchmark(parser, testcases);
  h_benchmark_report(stderr, res);
}

void register_benchmark_tests(void) {
  g_test_add_func("/core/benchmark/1", test_benchmark_1);
}
