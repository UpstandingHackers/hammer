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

#include "platform.h"

#include <stdio.h>
#include <string.h>
#include "hammer.h"
#include "internal.h"
#include <stdlib.h>
#include <inttypes.h>
#include <ctype.h>

typedef struct pp_state {
  int delta;
  int indent_amt;
  int at_bol;
} pp_state_t;

void h_pprint(FILE* stream, const HParsedToken* tok, int indent, int delta) {
  if (tok == NULL) {
    fprintf(stream, "(null)");
    return;
  }
  switch (tok->token_type) {
  case TT_NONE:
    fprintf(stream, "none");
    break;
  case TT_BYTES:
    fprintf(stream, "\"");
    for (size_t i = 0; i < tok->bytes.len; i++) {
      uint8_t c = tok->bytes.token[i];
      if (isprint(c))
        fputc(c, stream);
      else
        fprintf(stream, "\\%03hho", c);
    }
    fprintf(stream, "\"");
    break;
  case TT_SINT:
    if (tok->sint < 0)
      fprintf(stream, "-%#" PRIx64, -tok->sint);
    else
      fprintf(stream, "+%#" PRIx64, tok->sint);
    break;
  case TT_UINT:
    fprintf(stream, "%#" PRIx64, tok->uint);
    break;
  case TT_SEQUENCE:
    if (tok->seq->used == 0)
      fprintf(stream, "[ ]");
    else {
      fprintf(stream, "[%*s", delta - 1, "");
      for (size_t i = 0; i < tok->seq->used; i++) {
	if (i > 0) fprintf(stream, "\n%*s,%*s", indent, "", delta - 1, "");
        h_pprint(stream, tok->seq->elements[i], indent + delta, delta);
      }
      if (tok->seq->used > 2)
        fprintf(stream, "\n%*s]", indent, "");
      else
        fprintf(stream, " ]");
    }
    break;
  default:
    if(tok->token_type >= TT_USER) {
      const HTTEntry *e = h_get_token_type_entry(tok->token_type);
      fprintf(stream, "USER %d (%s) ", e->value - TT_USER, e->name);
      if (e->pprint)
        e->pprint(stream, tok, indent, delta);
    } else {
      assert_message(0, "Should not reach here.");
    }
  }
}

void h_pprintln(FILE* stream, const HParsedToken* tok) {
  h_pprint(stream, tok, 0, 2);
  fputc('\n', stream);
}


struct result_buf {
  char* output;
  size_t len;
  size_t capacity;
};

static inline bool ensure_capacity(struct result_buf *buf, int amt) {
  while (buf->len + amt >= buf->capacity) {
    buf->output = (&system_allocator)->realloc(&system_allocator, buf->output, buf->capacity *= 2);
    if (!buf->output) {
      return false;
    }
  }
  return true;
}

bool h_append_buf(struct result_buf *buf, const char* input, int len) {
  if (ensure_capacity(buf, len)) {
    memcpy(buf->output + buf->len, input, len);
    buf->len += len;
    return true;
  } else {
    return false;
  }
}

bool h_append_buf_c(struct result_buf *buf, char v) {
  if (ensure_capacity(buf, 1)) {
    buf->output[buf->len++] = v;
    return true;
  } else {
    return false;
  }
}

/** append a formatted string to the result buffer */
bool h_append_buf_formatted(struct result_buf *buf, char* format, ...)
{
  char* tmpbuf;
  int len;
  bool result;
  va_list ap;

  va_start(ap, format);
  len = h_platform_vasprintf(&tmpbuf, format, ap);
  result = h_append_buf(buf, tmpbuf, len);
  free(tmpbuf);
  va_end(ap);

  return result;
}

static void unamb_sub(const HParsedToken* tok, struct result_buf *buf) {
  if (!tok) {
    h_append_buf(buf, "NULL", 4);
    return;
  }
  switch (tok->token_type) {
  case TT_NONE:
    h_append_buf(buf, "null", 4);
    break;
  case TT_BYTES:
    if (tok->bytes.len == 0)
      h_append_buf(buf, "<>", 2);
    else {
      for (size_t i = 0; i < tok->bytes.len; i++) {
        const char *HEX = "0123456789abcdef";
        h_append_buf_c(buf, (i == 0) ? '<': '.');
        char c = tok->bytes.token[i];
        h_append_buf_c(buf, HEX[(c >> 4) & 0xf]);
        h_append_buf_c(buf, HEX[(c >> 0) & 0xf]);
      }
      h_append_buf_c(buf, '>');
    }
    break;
  case TT_SINT:
    if (tok->sint < 0)
      h_append_buf_formatted(buf, "s-%#" PRIx64, -tok->sint);
    else
      h_append_buf_formatted(buf, "s%#" PRIx64, tok->sint);
    break;
  case TT_UINT:
    h_append_buf_formatted(buf, "u%#" PRIx64, tok->uint);
    break;
  case TT_ERR:
    h_append_buf(buf, "ERR", 3);
    break;
  case TT_SEQUENCE: {
    h_append_buf_c(buf, '(');
    for (size_t i = 0; i < tok->seq->used; i++) {
      if (i > 0)
        h_append_buf_c(buf, ' ');
      unamb_sub(tok->seq->elements[i], buf);
    }
    h_append_buf_c(buf, ')');
  }
    break;
  default: {
    const HTTEntry *e = h_get_token_type_entry(tok->token_type);
    if (e) {
      h_append_buf_c(buf, '{');
      e->unamb_sub(tok, buf);
      h_append_buf_c(buf, '}');
    } else {
      assert_message(0, "Bogus token type.");
    }
  }
  }
}
  

char* h_write_result_unamb(const HParsedToken* tok) {
  struct result_buf buf = {
    .output = h_alloc(&system_allocator, 16),
    .len = 0,
    .capacity = 16
  };
  assert(buf.output != NULL);
  unamb_sub(tok, &buf);
  h_append_buf_c(&buf, 0);
  return buf.output;
}
  


