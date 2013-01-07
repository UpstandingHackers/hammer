#include <glib.h>
#include "test_suite.h"
#include "hammer.h"

static void test_tt_user(void) {
  g_check_cmpint(TT_USER, >, TT_NONE);
  g_check_cmpint(TT_USER, >, TT_BYTES);
  g_check_cmpint(TT_USER, >, TT_SINT);
  g_check_cmpint(TT_USER, >, TT_UINT);
  g_check_cmpint(TT_USER, >, TT_SEQUENCE);
  g_check_cmpint(TT_USER, >, TT_ERR);
}

void register_misc_tests(void) {
  g_test_add_func("/core/misc/tt_user", test_tt_user);
}
