// Example parser: Base64, with fine-grained semantic actions
//
// Demonstrates how to attach semantic actions to grammar rules and piece by
// piece transform the parse tree into the desired semantic representation,
// in this case a sequence of 8-bit values.
//
// Note how the grammar is defined by using the macros H_RULE and H_ARULE.
// Those rules using ARULE get an attached action which must be declared (as
// a function of type HAction) with a standard name based on the rule name.
//
// This variant of the example uses fine-grained semantic actions that
// transform the parse tree in small steps in a bottom-up fashion. Compare
// base64_sem2.c for an alternative approach using a single top-level action.

#include "../src/hammer.h"
#include "../src/glue.h"
#include <assert.h>


///
// Semantic actions for the grammar below, each corresponds to an "ARULE".
// They must be named act_<rulename>.
///

const HParsedToken *act_bsfdig(const HParseResult *p)
{
    HParsedToken *res = H_MAKE_UINT(0);

    uint8_t c = H_CAST_UINT(p->ast);

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

H_ACT_APPLY(act_index0, h_act_index, 0);

#define act_bsfdig_4bit  act_bsfdig
#define act_bsfdig_2bit  act_bsfdig

#define act_equals       h_act_ignore
#define act_ws           h_act_ignore

#define act_document     act_index0

// General-form action to turn a block of base64 digits into bytes.
const HParsedToken *act_base64_n(int n, const HParseResult *p)
{
    HParsedToken *res = H_MAKE_SEQN(n);

    HParsedToken **digits = h_seq_elements(p->ast);

    uint32_t x = 0;
    int bits = 0;
    for(int i=0; i<n+1; i++) {
        x <<= 6; x |= digits[i]->uint;
        bits += 6;
    }
    x >>= bits%8;   // align, i.e. cut off extra bits

    for(int i=0; i<n; i++) {
        HParsedToken *item = H_MAKE_UINT(x & 0xFF);

        res->seq->elements[n-1-i] = item;   // output the last byte and
        x >>= 8;                            // discard it
    }
    res->seq->used = n;

    return res;
}

H_ACT_APPLY(act_base64_3, act_base64_n, 3);
H_ACT_APPLY(act_base64_2, act_base64_n, 2);
H_ACT_APPLY(act_base64_1, act_base64_n, 1);

const HParsedToken *act_base64(const HParseResult *p)
{
    assert(p->ast->token_type == TT_SEQUENCE);
    assert(p->ast->seq->used == 2);
    assert(p->ast->seq->elements[0]->token_type == TT_SEQUENCE);

    HParsedToken *res = H_MAKE_SEQ();

    // concatenate base64_3 blocks
    HCountedArray *seq = H_FIELD_SEQ(0);
    for(size_t i=0; i<seq->used; i++)
        h_seq_append(res, seq->elements[i]);

    // append one trailing base64_2 or _1 block
    const HParsedToken *tok = h_seq_index(p->ast, 1);
    if(tok->token_type == TT_SEQUENCE)
        h_seq_append(res, tok);

    return res;
}


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
