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

#include <search.h>
#include <stdlib.h>
#include "hammer.h"
#include "internal.h"
#include "tsearch.h"

#if defined(_MSC_VER)
#define h_strdup _strdup
#else
#define h_strdup strdup
#endif

static void *tt_registry = NULL;
static HTTEntry** tt_by_id = NULL;
static unsigned int tt_by_id_sz = 0;
#define TT_START TT_USER
static HTokenType tt_next = TT_START;

/*
  // TODO: These are for the extension registry, which does not yet have a good name.
static void *ext_registry = NULL;
static HTTEntry** ext_by_id = NULL;
static int ext_by_id_sz = 0;
static int ext_next = 0;
*/


static int compare_entries(const void* v1, const void* v2) {
  const HTTEntry *e1 = (HTTEntry*)v1, *e2 = (HTTEntry*)v2;
  return strcmp(e1->name, e2->name);
}

static void default_unamb_sub(const HParsedToken* tok,
                              struct result_buf* buf) {
  h_append_buf_formatted(buf, "XXX AMBIGUOUS USER TYPE %d", tok->token_type);
}

HTokenType h_allocate_token_new(
    const char* name,
    void (*unamb_sub)(const HParsedToken *tok, struct result_buf *buf),
    void (*pprint)(FILE* stream, const HParsedToken* tok, int indent, int delta)) {
  HTTEntry* new_entry = h_alloc(&system_allocator, sizeof(*new_entry));
  assert(new_entry != NULL);
  new_entry->name = name;
  new_entry->value = 0;
  new_entry->unamb_sub = unamb_sub ? unamb_sub : default_unamb_sub;
  new_entry->pprint = pprint;
  HTTEntry* probe = *(HTTEntry**)tsearch(new_entry, &tt_registry, compare_entries);
  if (probe->value != 0) {
    // Token type already exists...
    // TODO: treat this as a bug?
    (&system_allocator)->free(&system_allocator, new_entry);
    return probe->value;
  } else {
    // new value
    probe->name = h_strdup(probe->name); // drop ownership of name
    probe->value = tt_next++;
    if ((probe->value - TT_START) >= tt_by_id_sz) {
      if (tt_by_id_sz == 0) {
	tt_by_id = malloc(sizeof(*tt_by_id) * ((tt_by_id_sz = (tt_next - TT_START) * 16)));
      } else {
	tt_by_id = realloc(tt_by_id, sizeof(*tt_by_id) * ((tt_by_id_sz *= 2)));
      }
      if (!tt_by_id) {
	return TT_INVALID;
      }
    }
    assert(probe->value - TT_START < tt_by_id_sz);
    tt_by_id[probe->value - TT_START] = probe;
    return probe->value;
  }
}
HTokenType h_allocate_token_type(const char* name) {
  return h_allocate_token_new(name, NULL, NULL);
}
HTokenType h_get_token_type_number(const char* name) {
  HTTEntry e;
  e.name = name;
  HTTEntry **ret = (HTTEntry**)tfind(&e, &tt_registry, compare_entries);
  if (ret == NULL)
    return 0;
  else
    return (*ret)->value;
}
const char* h_get_token_type_name(HTokenType token_type) {
  if (token_type >= tt_next || token_type < TT_START)
    return NULL;
  else
    return tt_by_id[token_type - TT_START]->name;
}
const HTTEntry* h_get_token_type_entry(HTokenType token_type) {
  if (token_type >= tt_next || token_type < TT_START)
    return NULL;
  else
    return tt_by_id[token_type - TT_START];
}
