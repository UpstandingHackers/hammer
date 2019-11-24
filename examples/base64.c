// Example parser: Base64, syntax only.
//
// Demonstrates how to construct a Hammer parser that recognizes valid Base64
// sequences.
//
// Note that no semantic evaluation of the sequence is performed, i.e. the
// byte sequence being represented is not returned, or determined. See
// base64_sem1.c and base64_sem2.c for examples how to attach appropriate
// semantic actions to the grammar.

#include <inttypes.h>
#include "../src/hammer.h"

const HParser* document = NULL;

void init_parser(void)
{
    // CORE
    HParser *digit = h_ch_range(0x30, 0x39);
    HParser *alpha = h_choice(h_ch_range(0x41, 0x5a), h_ch_range(0x61, 0x7a), NULL);

    // AUX.
    HParser *plus = h_ch('+');
    HParser *slash = h_ch('/');
    HParser *equals = h_ch('=');

    HParser *bsfdig = h_choice(alpha, digit, plus, slash, NULL);
    HParser *bsfdig_4bit = h_in((uint8_t *)"AEIMQUYcgkosw048", 16);
    HParser *bsfdig_2bit = h_in((uint8_t *)"AQgw", 4);
    HParser *base64_3 = h_repeat_n(bsfdig, 4);
    HParser *base64_2 = h_sequence(bsfdig, bsfdig, bsfdig_4bit, equals, NULL);
    HParser *base64_1 = h_sequence(bsfdig, bsfdig_2bit, equals, equals, NULL);
    HParser *base64 = h_sequence(h_many(base64_3),
                                       h_optional(h_choice(base64_2,
                                                           base64_1, NULL)),
                                       NULL);

    document = h_sequence(h_whitespace(base64), h_whitespace(h_end_p()), NULL);
}


#include <stdio.h>

int main(int argc, char **argv)
{
    uint8_t input[102400];
    size_t inputsize;
    const HParseResult *result;

    init_parser();

    inputsize = fread(input, 1, sizeof(input), stdin);
    fprintf(stderr, "inputsize=%zu\ninput=", inputsize);
    fwrite(input, 1, inputsize, stderr);
    result = h_parse(document, input, inputsize);

    if(result) {
        fprintf(stderr, "parsed=%" PRId64 " bytes\n", result->bit_length/8);
        h_pprint(stdout, result->ast, 0, 0);
        return 0;
    } else {
        return 1;
    }
}
