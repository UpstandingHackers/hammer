#include <glib.h>
#include "hammer.h"
#include "internal.h"
#include "cfgrammar.h"
#include "test_suite.h"

static void test_end(void) {
  const HParser *p = h_end_p();
  HCFGrammar *g = h_cfgrammar(&system_allocator, p);

  g_check_hashtable_size(g->nts, 1);
  g_check_hashtable_size(g->geneps, 0);

  g_check_derives_epsilon_not(g, p);
}

static void test_example_1(void) {
  const HParser *c = h_many(h_ch('x'));
  const HParser *q = h_sequence(c, h_ch('y'), NULL);
  const HParser *p = h_choice(q, h_end_p(), NULL);
  HCFGrammar *g = h_cfgrammar(&system_allocator, p);

  g_check_nonterminal(g, c);
  g_check_nonterminal(g, q);
  g_check_nonterminal(g, p);

  g_check_derives_epsilon(g, c);
  g_check_derives_epsilon_not(g, q);
  g_check_derives_epsilon_not(g, p);

  g_check_firstset_present(1, g, p, "$");
  g_check_firstset_present(1, g, p, "x");
  g_check_firstset_present(1, g, p, "y");

  g_check_followset_absent(1, g, c, "$");
  g_check_followset_absent(1, g, c, "x");
  g_check_followset_present(1, g, c, "y");
}

void register_grammar_tests(void) {
  g_test_add_func("/core/grammar/end", test_end);
  g_test_add_func("/core/grammar/example_1", test_example_1);
}
