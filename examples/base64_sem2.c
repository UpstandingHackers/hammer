// Example parser: Base64, with fine-grained semantic actions
//
// Demonstrates how to attach semantic actions to a grammar and transform the
// parse tree into the desired semantic representation, in this case a sequence
// of 8-bit values.
//
// Note how the grammar is defined by using the macros H_RULE and H_ARULE.
// Those rules using ARULE get an attached action which must be declared (as
// a function of type HAction) with a standard name based on the rule name.
//
// This variant of the example uses coarse-grained semantic actions,
// transforming the entire parse tree in one big step. Compare base64_sem1.c
// for an alternative approach using a fine-grained piece-by-piece
// transformation.

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

// helper: return the numeric value of a parsed base64 digit
uint8_t bsfdig_value(const HParsedToken *p)
{
    uint8_t value = 0;

    if(p && p->token_type == TT_UINT) {
        uint8_t c = p->uint;
        if(c >= 0x40 && c <= 0x5A) // A-Z
            value = c - 0x41;
        else if(c >= 0x60 && c <= 0x7A) // a-z
            value = c - 0x61 + 26;
        else if(c >= 0x30 && c <= 0x39) // 0-9
            value = c - 0x30 + 52;
        else if(c == '+')
            value = 62;
        else if(c == '/')
            value = 63;
    }

    return value;
}

// helper: append a byte value to a sequence
void seq_append_byte(HCountedArray *a, uint8_t b)
{
    HParsedToken *item = h_arena_malloc(a->arena, sizeof(HParsedToken));
    item->token_type = TT_UINT;
    item->uint = b;
    h_carray_append(a, item);
}

const HParsedToken *act_base64(const HParseResult *p)
{
    assert(p->ast->token_type == TT_SEQUENCE);
    assert(p->ast->seq->used == 2);
    assert(p->ast->seq->elements[0]->token_type == TT_SEQUENCE);

    // grab b64_3 block sequence
    // grab and analyze b64 end block (_2 or _1)
    const HParsedToken *b64_3 = p->ast->seq->elements[0];
    const HParsedToken *b64_2 = p->ast->seq->elements[1];
    const HParsedToken *b64_1 = p->ast->seq->elements[1];

    if(b64_2->token_type != TT_SEQUENCE)
        b64_1 = b64_2 = NULL;
    else if(b64_2->seq->elements[2]->uint == '=')
        b64_2 = NULL;
    else
        b64_1 = NULL;

    // allocate result sequence
    HParsedToken *res = h_arena_malloc(p->arena, sizeof(HParsedToken));
    res->token_type = TT_SEQUENCE;
    res->seq = h_carray_new(p->arena);

    // concatenate base64_3 blocks
    for(size_t i=0; i<b64_3->seq->used; i++) {
        assert(b64_3->seq->elements[i]->token_type == TT_SEQUENCE);
        HParsedToken **digits = b64_3->seq->elements[i]->seq->elements;

        uint32_t x = bsfdig_value(digits[0]);
        x <<= 6; x |= bsfdig_value(digits[1]);
        x <<= 6; x |= bsfdig_value(digits[2]);
        x <<= 6; x |= bsfdig_value(digits[3]);
        seq_append_byte(res->seq, (x >> 16) & 0xFF);
        seq_append_byte(res->seq, (x >> 8) & 0xFF);
        seq_append_byte(res->seq, x & 0xFF);
    }

    // append one trailing base64_2 or _1 block
    if(b64_2) {
        HParsedToken **digits = b64_2->seq->elements;
        uint32_t x = bsfdig_value(digits[0]);
        x <<= 6; x |= bsfdig_value(digits[1]);
        x <<= 6; x |= bsfdig_value(digits[2]);
        seq_append_byte(res->seq, (x >> 10) & 0xFF);
        seq_append_byte(res->seq, (x >> 2) & 0xFF);
    } else if(b64_1) {
        HParsedToken **digits = b64_1->seq->elements;
        uint32_t x = bsfdig_value(digits[0]);
        x <<= 6; x |= bsfdig_value(digits[1]);
        seq_append_byte(res->seq, (x >> 4) & 0xFF);
    }

    return res;
}

#define act_ws           act_ignore
#define act_document     act_index0


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
    H_RULE (equals,  h_ch('='));

    H_RULE (bsfdig,       h_choice(alpha, digit, plus, slash, NULL));
    H_RULE (bsfdig_4bit,  h_in((uint8_t *)"AEIMQUYcgkosw048", 16));
    H_RULE (bsfdig_2bit,  h_in((uint8_t *)"AQgw", 4));
    H_RULE (base64_3,     h_repeat_n(bsfdig, 4));
    H_RULE (base64_2,     h_sequence(bsfdig, bsfdig, bsfdig_4bit, equals, NULL));
    H_RULE (base64_1,     h_sequence(bsfdig, bsfdig_2bit, equals, equals, NULL));
    H_ARULE(base64,       h_sequence(h_many(base64_3),
                                     h_optional(h_choice(base64_2,
                                                         base64_1, NULL)),
                                     NULL));

    H_ARULE(ws,           h_many(space));
    H_ARULE(document,     h_sequence(ws, base64, ws, h_end_p(), NULL));

    // BUG sometimes inputs that should just don't parse.
    // It *seemed* to happen mostly with things like "bbbbaaaaBA==".
    // Using less actions seemed to make it less likely.

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