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

#include <glib.h>
#include "hammer.h"
#include "test_suite.h"

extern void register_bitreader_tests();
extern void register_bitwriter_tests();
extern void register_parser_tests();
extern void register_grammar_tests();
extern void register_misc_tests();
extern void register_benchmark_tests();

int main(int argc, char** argv) {
  g_test_init(&argc, &argv, NULL);

  // register various test suites...
  register_bitreader_tests();
  register_bitwriter_tests();
  register_parser_tests();
  register_grammar_tests();
  register_misc_tests();
  register_benchmark_tests();

  g_test_run();
}

