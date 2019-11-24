#include "parser_internal.h"

typedef struct {
  ssize_t offset;
  int whence;
} HSeek;

static HParseResult *parse_skip(void *env, HParseState *state)
{
  size_t n = (uintptr_t)env;

  h_skip_bits(&state->input_stream, n);
  return make_result(state->arena, NULL);
}

static HParseResult *parse_seek(void *env, HParseState *state)
{
  HSeek *s = (HSeek *)env;
  HInputStream *stream = &state->input_stream;
  size_t pos;

  /* determine base position */
  switch (s->whence) {
  case SEEK_SET:
    pos = 0;
    break;
  case SEEK_END:
    pos = h_input_stream_length(stream);
    break;
  case SEEK_CUR:
    pos = h_input_stream_pos(stream);
    break;
  default:
    return NULL;	/* invalid argument */
  }

  /* calculate target position and do basic overflow checks */
  if (s->offset < 0 && (size_t)(- s->offset) > pos)
    return NULL;	/* underflow */
  if (s->offset > 0 && SIZE_MAX - s->offset < pos)
    return NULL;	/* overflow */
  pos += s->offset;

  /* perform the seek and check for overrun */
  h_seek_bits(stream, pos);
  if (stream->overrun)
    return NULL;

  HParsedToken *tok = a_new(HParsedToken, 1);
  tok->token_type = TT_UINT;
  tok->uint = pos;
  return make_result(state->arena, tok);
}

static HParseResult *parse_tell(void *env, HParseState *state)
{
  HParsedToken *tok = a_new(HParsedToken, 1);
  tok->token_type = TT_UINT;
  tok->uint = h_input_stream_pos(&state->input_stream);
  return make_result(state->arena, tok);
}

static const HParserVtable skip_vt = {
  .parse = parse_skip,
  .isValidRegular = h_false,
  .isValidCF = h_false,
  .compile_to_rvm = h_not_regular,
  .higher = false,
};

static const HParserVtable seek_vt = {
  .parse = parse_seek,
  .isValidRegular = h_false,
  .isValidCF = h_false,
  .compile_to_rvm = h_not_regular,
  .higher = false,
};

static const HParserVtable tell_vt = {
  .parse = parse_tell,
  .isValidRegular = h_false,
  .isValidCF = h_false,
  .compile_to_rvm = h_not_regular,
  .higher = false,
};

HParser* h_skip(size_t n)
{
  return h_skip__m(&system_allocator, n);
}

HParser *h_skip__m(HAllocator* mm__, size_t n)
{
  return h_new_parser(mm__, &skip_vt, (void *)n);
}

HParser* h_seek(ssize_t offset, int whence)
{
  return h_seek__m(&system_allocator, offset, whence);
}

HParser *h_seek__m(HAllocator* mm__, ssize_t offset, int whence)
{
  HSeek *env = h_new(HSeek, 1);
  env->offset = offset;
  env->whence = whence;
  return h_new_parser(mm__, &seek_vt, env);
}

HParser *h_tell()
{
  return h_tell__m(&system_allocator);
}

HParser *h_tell__m(HAllocator* mm__)
{
  return h_new_parser(mm__, &tell_vt, NULL);
}
