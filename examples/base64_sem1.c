#include "../src/hammer.h"
#include "../src/internal.h"    // for h_carray functions (XXX ?!)
#include <assert.h>


#define H_RULE(rule, def) const HParser *rule = def
#define H_ARULE(rule, def) const HParser *rule = h_action(def, act_ ## rule)


///
// Semantic action helpers.
// These might be candidates for inclusion in the library.
///

// The action equivalent of h_ignore.
const HParsedToken *act_ignore(const HParseResult *p)
{
    return NULL;
}

// Helper to build HAction's that pick one index out of a sequence.
const HParsedToken *act_index(int i, const HParseResult *p)
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

const HParsedToken *act_index0(const HParseResult *p)
{
    return act_index(0, p);
}


///
// Semantic actions for the grammar below, each corresponds to an "ARULE".
// They must be named act_<rulename>.
///

const HParsedToken *act_bsfdig(const HParseResult *p)
{
    HParsedToken *res = h_arena_malloc(p->arena, sizeof(HParsedToken));

    assert(p->ast->token_type == TT_UINT);
    uint8_t c = p->ast->uint;

    res->token_type = TT_UINT;
    if(c >= 0x40 && c <= 0x5A) // A-Z
        res->uint = c - 0x41;
    else if(c >= 0x60 && c <= 0x7A) // a-z
        res->uint = c - 0x61 + 26;
    else if(c >= 0x30 && c <= 0x39) // 0-9
        res->uint = c - 0x30 + 52;
    else if(c == '+')
        res->uint = 62;
    else if(c == '/')
        res->uint = 63;

    return res;
}

#define act_bsfdig_4bit  act_bsfdig
#define act_bsfdig_2bit  act_bsfdig

#define act_equals       act_ignore
#define act_ws           act_ignore

#define act_document     act_index0

// helper
void carray_append_uint(HCountedArray *array, uint8_t value)
{
    HParsedToken *item = h_arena_malloc(array->arena, sizeof(HParsedToken));
    item->token_type = TT_UINT;
    item->uint = value;
    h_carray_append(array, item);
}

const HParsedToken *act_base64_3(const HParseResult *p)
{
    assert(p->ast->token_type == TT_SEQUENCE);

    HParsedToken *res = h_arena_malloc(p->arena, sizeof(HParsedToken));
    res->token_type = TT_SEQUENCE;
    res->seq = h_carray_new_sized(p->arena, 4);

    HParsedToken **digits = p->ast->seq->elements;
    uint32_t x = digits[0]->uint;
    x <<= 6; x |= digits[1]->uint;
    x <<= 6; x |= digits[2]->uint;
    x <<= 6; x |= digits[3]->uint;

    carray_append_uint(res->seq, (x >> 16) & 0xFF);
    carray_append_uint(res->seq, (x >> 8) & 0xFF);
    carray_append_uint(res->seq, x & 0xFF);

    return res;
}

const HParsedToken *act_base64_2(const HParseResult *p)
{
    assert(p->ast->token_type == TT_SEQUENCE);

    HParsedToken *res = h_arena_malloc(p->arena, sizeof(HParsedToken));
    res->token_type = TT_SEQUENCE;
    res->seq = h_carray_new_sized(p->arena, 4);

    HParsedToken **digits = p->ast->seq->elements;
    uint32_t x = digits[0]->uint;
    x <<= 6; x |= digits[1]->uint;
    x <<= 6; x |= digits[2]->uint;

    carray_append_uint(res->seq, (x >> 10) & 0xFF);
    carray_append_uint(res->seq, (x >> 2) & 0xFF);

    return res;
}

const HParsedToken *act_base64_1(const HParseResult *p)
{
    assert(p->ast->token_type == TT_SEQUENCE);

    HParsedToken *res = h_arena_malloc(p->arena, sizeof(HParsedToken));
    res->token_type = TT_SEQUENCE;
    res->seq = h_carray_new_sized(p->arena, 4);

    HParsedToken **digits = p->ast->seq->elements;
    uint32_t x = digits[0]->uint;
    x <<= 6; x |= digits[1]->uint;

    carray_append_uint(res->seq, (x >> 4) & 0xFF);

    return res;
}

#if 0
const HParsedToken *act_base64(const HParseResult *p)
{
    // XXX
    // concatenate base64_3 blocks
    // append trailing base64_2 or _1 block
}
#endif


///
// Set up the parser with the grammar to be recognized.
///

const HParser *init_parser(void)
{
    // CORE
    H_RULE (digit,   h_ch_range(0x30, 0x39));
    H_RULE (alpha,   h_choice(h_ch_range(0x41, 0x5a), h_ch_range(0x61, 0x7a), NULL));
    H_RULE (space,   h_in((uint8_t *)" \t\n\r\f\v", 6));

    // AUX.
    H_RULE (plus,    h_ch('+'));
    H_RULE (slash,   h_ch('/'));
    H_ARULE(equals,  h_ch('='));

    H_ARULE(bsfdig,       h_choice(alpha, digit, plus, slash, NULL));
    H_ARULE(bsfdig_4bit,  h_in((uint8_t *)"AEIMQUYcgkosw048", 16));
    H_ARULE(bsfdig_2bit,  h_in((uint8_t *)"AQgw", 4));
    H_ARULE(base64_3,     h_repeat_n(bsfdig, 4));
    H_ARULE(base64_2,     h_sequence(bsfdig, bsfdig, bsfdig_4bit, equals, NULL));
    H_ARULE(base64_1,     h_sequence(bsfdig, bsfdig_2bit, equals, equals, NULL));
    H_RULE(base64,       h_sequence(h_many(base64_3),
                                     h_optional(h_choice(base64_2,
                                                         base64_1, NULL)),
                                     NULL));

    H_ARULE(ws,           h_many(space));
    H_ARULE(document,     h_sequence(ws, base64, ws, h_end_p(), NULL));

    return document;
}


///
// Main routine: print input, parse, print result, return success/failure.
///

#include <stdio.h>

int main(int argc, char **argv)
{
    uint8_t input[102400];
    size_t inputsize;
    const HParser *parser;
    const HParseResult *result;

    parser = init_parser();

    inputsize = fread(input, 1, sizeof(input), stdin);
    fprintf(stderr, "inputsize=%lu\ninput=", inputsize);
    fwrite(input, 1, inputsize, stderr);
    result = h_parse(parser, input, inputsize);

    if(result) {
        fprintf(stderr, "parsed=%lld bytes\n", result->bit_length/8);
        h_pprint(stdout, result->ast, 0, 0);
        return 0;
    } else {
        return 1;
    }
}