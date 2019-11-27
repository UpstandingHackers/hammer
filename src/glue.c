#include <assert.h>
#include "glue.h"
#include "hammer.h"
#include "internal.h"  // for h_carray_*
#include "parsers/parser_internal.h"

// Helper to build HAction's that pick one index out of a sequence.
HParsedToken *h_act_index(int i, const HParseResult *p, void* user_data)
{
    if(!p) return NULL;

    const HParsedToken *tok = p->ast;

    if(!tok || tok->token_type != TT_SEQUENCE)
        return NULL;

    const HCountedArray *seq = tok->seq;
    size_t n = seq->used;

    if(i<0 || (size_t)i>=n)
        return NULL;
    else
        return tok->seq->elements[i];
}

HParsedToken *h_act_first(const HParseResult *p, void* user_data) {
  assert(p->ast);
  assert(p->ast->token_type == TT_SEQUENCE);
  assert(p->ast->seq->used > 0);

  return p->ast->seq->elements[0];
}

HParsedToken *h_act_second(const HParseResult *p, void* user_data) {
  assert(p->ast);
  assert(p->ast->token_type == TT_SEQUENCE);
  assert(p->ast->seq->used > 0);

  return p->ast->seq->elements[1];
}

HParsedToken *h_act_last(const HParseResult *p, void* user_data) {
  assert(p->ast);
  assert(p->ast->token_type == TT_SEQUENCE);
  assert(p->ast->seq->used > 0);

  return p->ast->seq->elements[p->ast->seq->used-1];
}

static void act_flatten_(HCountedArray *seq, const HParsedToken *tok) {
  if(tok == NULL) {
    return;
  } else if(tok->token_type == TT_SEQUENCE) {
    size_t i;
    for(i=0; i<tok->seq->used; i++)
      act_flatten_(seq, tok->seq->elements[i]);
  } else {
    h_carray_append(seq, (HParsedToken *)tok);
  }
}

HParsedToken *h_act_flatten(const HParseResult *p, void* user_data) {
  HCountedArray *seq = h_carray_new(p->arena);

  act_flatten_(seq, p->ast);

  HParsedToken *res = a_new_(p->arena, HParsedToken, 1);
  res->token_type = TT_SEQUENCE;
  res->seq = seq;
  res->index = p->ast->index;
  res->bit_offset = p->ast->bit_offset;
  return res;
}

HParsedToken *h_act_ignore(const HParseResult *p, void* user_data) {
  return NULL;
}

// Low-level helper for the h_make family.
HParsedToken *h_make_(HArena *arena, HTokenType type)
{
  HParsedToken *ret = h_arena_malloc(arena, sizeof(HParsedToken));
  ret->token_type = type;
  return ret;
}

HParsedToken *h_make(HArena *arena, HTokenType type, void *value)
{
  assert(type >= TT_USER);
  HParsedToken *ret = h_make_(arena, type);
  ret->user = value;
  return ret;
}

HParsedToken *h_make_seq(HArena *arena)
{
  HParsedToken *ret = h_make_(arena, TT_SEQUENCE);
  ret->seq = h_carray_new(arena);
  return ret;
}

HParsedToken *h_make_seqn(HArena *arena, size_t n)
{
  HParsedToken *ret = h_make_(arena, TT_SEQUENCE);
  ret->seq = h_carray_new_sized(arena, n);
  return ret;
}

HParsedToken *h_make_bytes(HArena *arena, const uint8_t *array, size_t len)
{
  HParsedToken *ret = h_make_(arena, TT_BYTES);
  ret->bytes.len = len;
  ret->bytes.token = array;
  return ret;
}

HParsedToken *h_make_sint(HArena *arena, int64_t val)
{
  HParsedToken *ret = h_make_(arena, TT_SINT);
  ret->sint = val;
  return ret;
}

HParsedToken *h_make_uint(HArena *arena, uint64_t val)
{
  HParsedToken *ret = h_make_(arena, TT_UINT);
  ret->uint = val;
  return ret;
}

// XXX -> internal
HParsedToken *h_carray_index(const HCountedArray *a, size_t i)
{
  assert(i < a->used);
  return a->elements[i];
}

size_t h_seq_len(const HParsedToken *p)
{
  assert(p != NULL);
  assert(p->token_type == TT_SEQUENCE);
  return p->seq->used;
}

HParsedToken **h_seq_elements(const HParsedToken *p)
{
  assert(p != NULL);
  assert(p->token_type == TT_SEQUENCE);
  return p->seq->elements;
}

HParsedToken *h_seq_index(const HParsedToken *p, size_t i)
{
  assert(p != NULL);
  assert(p->token_type == TT_SEQUENCE);
  return h_carray_index(p->seq, i);
}

HParsedToken *h_seq_index_path(const HParsedToken *p, size_t i, ...)
{
  va_list va;

  va_start(va, i);
  HParsedToken *ret = h_seq_index_vpath(p, i, va);
  va_end(va);

  return ret;
}

HParsedToken *h_seq_index_vpath(const HParsedToken *p, size_t i, va_list va)
{
  HParsedToken *ret = h_seq_index(p, i);
  int j;

  while((j = va_arg(va, int)) >= 0)
    ret = h_seq_index(ret, j);

  return ret;
}

void h_seq_snoc(HParsedToken *xs, const HParsedToken *x)
{
  assert(xs != NULL);
  assert(xs->token_type == TT_SEQUENCE);

  h_carray_append(xs->seq, (HParsedToken *)x);
}

void h_seq_append(HParsedToken *xs, const HParsedToken *ys)
{
  assert(xs != NULL);
  assert(xs->token_type == TT_SEQUENCE);
  assert(ys != NULL);
  assert(ys->token_type == TT_SEQUENCE);

  for(size_t i=0; i<ys->seq->used; i++)
    h_carray_append(xs->seq, ys->seq->elements[i]);
}

// Flatten nested sequences. Always returns a sequence.
// If input element is not a sequence, returns it as a singleton sequence.
const HParsedToken *h_seq_flatten(HArena *arena, const HParsedToken *p)
{
  assert(p != NULL);

  HParsedToken *ret = h_make_seq(arena);
  switch(p->token_type) {
  case TT_SEQUENCE:
    // Flatten and append all.
    for(size_t i = 0; i<p->seq->used; i++) {
      h_seq_append(ret, h_seq_flatten(arena, h_seq_index(p, i)));
    }
    break;
  default:
    // Make singleton sequence.
    h_seq_snoc(ret, p);
    break;
  }

  return ret;
}
