/* Pretty-printer for Hammer.
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

#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include "hammer.h"
#include "internal.h"
#include <stdlib.h>

typedef struct pp_state {
  int delta;
  int indent_amt;
  int at_bol;
} pp_state_t;

void h_pprint(FILE* stream, const HParsedToken* tok, int indent, int delta) {
  switch (tok->token_type) {
  case TT_NONE:
    fprintf(stream, "%*snull\n", indent, "");
    break;
  case TT_BYTES:
    if (tok->bytes.len == 0)
      fprintf(stream, "%*s<>\n", indent, "");
    else {
      fprintf(stream, "%*s", indent, "");
      for (size_t i = 0; i < tok->bytes.len; i++) {
	fprintf(stream,
		"%c%02hhx",
		(i == 0) ? '<' : '.',
		tok->bytes.token[i]);
      }
      fprintf(stream, ">\n");
    }
    break;
  case TT_SINT:
    if (tok->sint < 0)
      fprintf(stream, "%*ss -%#lx\n", indent, "", -tok->sint);
    else
      fprintf(stream, "%*ss %#lx\n", indent, "", tok->sint);
      
    break;
  case TT_UINT:
    fprintf(stream, "%*su %#lx\n", indent, "", tok->uint);
    break;
  case TT_SEQUENCE: {
    fprintf(stream, "%*s[\n", indent, "");
    for (size_t i = 0; i < tok->seq->used; i++) {
      h_pprint(stream, tok->seq->elements[i], indent + delta, delta);
    }
    fprintf(stream, "%*s]\n", indent, "");
  }
    break;
  case TT_USER:
    fprintf(stream, "%*sUSER\n", indent, "");
    break;
  default:
    if(tok->token_type > TT_USER) {
      fprintf(stream, "%*sUSER %d\n", indent, "", tok->token_type-TT_USER);
    } else {
      assert_message(0, "Should not reach here.");
    }
  }
}


struct result_buf {
  char* output;
  HAllocator *mm__;
  size_t len;
  size_t capacity;
};

static inline void ensure_capacity(struct result_buf *buf, int amt) {
  while (buf->len + amt >= buf->capacity)
    buf->output = buf->mm__->realloc(buf->mm__, buf->output, buf->capacity *= 2);
}

static inline void append_buf(struct result_buf *buf, const char* input, int len) {
  ensure_capacity(buf, len);
  memcpy(buf->output + buf->len, input, len);
  buf->len += len;
}

static inline void append_buf_c(struct result_buf *buf, char v) {
  ensure_capacity(buf, 1);
  buf->output[buf->len++] = v;
}

static void unamb_sub(const HParsedToken* tok, struct result_buf *buf) {
  char* tmpbuf;
  int len;
  if (!tok) {
    append_buf(buf, "NULL", 4);
    return;
  }
  switch (tok->token_type) {
  case TT_NONE:
    append_buf(buf, "null", 4);
    break;
  case TT_BYTES:
    if (tok->bytes.len == 0)
      append_buf(buf, "<>", 2);
    else {
      for (size_t i = 0; i < tok->bytes.len; i++) {
	const char *HEX = "0123456789abcdef";
	append_buf_c(buf, (i == 0) ? '<': '.');
	char c = tok->bytes.token[i];
	append_buf_c(buf, HEX[(c >> 4) & 0xf]);
	append_buf_c(buf, HEX[(c >> 0) & 0xf]);
      }
      append_buf_c(buf, '>');
    }
    break;
  case TT_SINT:
    if (tok->sint < 0)
      len = asprintf(&tmpbuf, "s-%#lx", -tok->sint);
    else
      len = asprintf(&tmpbuf, "s%#lx", tok->sint);
    append_buf(buf, tmpbuf, len);
    free(tmpbuf);
    break;
  case TT_UINT:
    len = asprintf(&tmpbuf, "u%#lx", tok->uint);
    append_buf(buf, tmpbuf, len);
    free(tmpbuf);
    break;
  case TT_ERR:
    append_buf(buf, "ERR", 3);
    break;
  case TT_SEQUENCE: {
    append_buf_c(buf, '(');
    for (size_t i = 0; i < tok->seq->used; i++) {
      if (i > 0)
	append_buf_c(buf, ' ');
      unamb_sub(tok->seq->elements[i], buf);
    }
    append_buf_c(buf, ')');
  }
    break;
  default:
    fprintf(stderr, "Unexpected token type %d\n", tok->token_type);
    assert_message(0, "Should not reach here.");
  }
}
  

char* h_write_result_unamb(const HParsedToken* tok) {
  return h_write_result_unamb__m(&system_allocator, tok);
}
char* h_write_result_unamb__m(HAllocator* mm__, const HParsedToken* tok) {
  struct result_buf buf = {
    .output = mm__->alloc(mm__, 16),
    .len = 0,
    .mm__ = mm__,
    .capacity = 16
  };
  unamb_sub(tok, &buf);
  append_buf_c(&buf, 0);
  return buf.output;
}
  


