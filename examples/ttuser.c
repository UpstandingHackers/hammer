/*
 * Example parser that demonstrates the use of user-defined token types.
 *
 * Note the custom printer function that hooks into h_pprint().
 */

#include "../src/hammer.h"
#include "../src/glue.h"


/*
 * custom tokens
 */

HTokenType TT_SUBJ, TT_PRED, TT_OBJ, TT_ADJ, TT_ADVC;

void
pprint(FILE *stream, const HParsedToken *tok, int indent, int delta)
{
	/* 
	 * Pretty-printer rules:
	 *
	 *  - Output 'indent' spaces after every newline you produce.
	 *  - Do not add indent on the first line of output.
	 *  - Do not add a trailing newline.
	 *  - Indent sub-objects by adding 'delta' to 'indent'.
	 */

	if (((HParsedToken *)tok->user)->token_type == TT_SEQUENCE)
		fprintf(stream, "\n%*s", indent, "");
	h_pprint(stream, tok->user, indent, delta);
}

/* XXX define umamb_sub as well */

void
init(void)
{
	TT_SUBJ = h_allocate_token_new("subject", NULL, pprint);
	TT_PRED = h_allocate_token_new("predicate", NULL, pprint);
	TT_OBJ  = h_allocate_token_new("object", NULL, pprint);
	TT_ADJ  = h_allocate_token_new("adjective", NULL, pprint);
	TT_ADVC = h_allocate_token_new("adverbial clause", NULL, pprint);
}


/*
 * semantic actions
 *
 * Normally these would be more interesting, but for this example, we just wrap
 * our tokens in their intended types.
 */
HParsedToken *act_subj(const HParseResult *p, void *u) {
	return H_MAKE(SUBJ, (void *)p->ast);
}
HParsedToken *act_pred(const HParseResult *p, void *u) {
	return H_MAKE(PRED, (void *)p->ast);
}
HParsedToken *act_obj(const HParseResult *p, void *u) {
	return H_MAKE(OBJ, (void *)p->ast);
}
HParsedToken *act_adj(const HParseResult *p, void *u) {
	return H_MAKE(ADJ, (void *)p->ast);
}
HParsedToken *act_advc(const HParseResult *p, void *u) {
	return H_MAKE(ADVC, (void *)p->ast);
}


/*
 * grammar
 */

HParser *
build_parser(void)
{
	/* words */
	#define W(X)	h_whitespace(h_literal(#X))
	H_RULE(art,	h_choice(W(a), W(the), NULL));
	H_RULE(noun,	h_choice(W(cat), W(dog), W(fox), W(tiger), W(lion),
			    W(bear), W(fence), W(tree), W(car), W(cow), NULL));
	H_RULE(verb,	h_choice(W(eats), W(jumps), W(falls), NULL));
	H_ARULE(adj,	h_choice(W(quick), W(slow), W(happy), W(lazy), W(cyan),
			    W(magenta), W(yellow), W(black), W(brown), NULL));
	H_RULE(adverb,	h_choice(W(with), W(over), W(after), NULL));
	#undef W

	/* phrases */
	H_RULE(nphrase,	h_sequence(art, h_many(adj), noun, NULL));

	/* sentence structure */
	H_ARULE(subj,	nphrase);
	H_ARULE(pred,	verb);
	H_ARULE(obj,	nphrase);
	H_ARULE(advc,	h_sequence(adverb, nphrase, NULL));
	H_RULE(sentnc,	h_sequence(subj, pred,
			    h_optional(obj), h_optional(advc), NULL));

	return sentnc;
}


/*
 * main routine: read, parse, print
 *
 * input e.g.:
 * "the quick brown fox jumps the fence with a cyan lion"
 */

#include <stdio.h>
#include <inttypes.h>

int
main(int argc, char **argv)
{
	uint8_t input[1024];
	size_t sz;
	const HParser *parser;
	const HParseResult *result;

	init();
	parser = build_parser();

	sz = fread(input, 1, sizeof(input), stdin);
	if (!feof(stdin)) {
		fprintf(stderr, "too much input\n");
		return 1;
	}

	result = h_parse(parser, input, sz);
	if (!result) {
		fprintf(stderr, "no parse\n");
		return 1;
	}

        h_pprintln(stdout, result->ast);
        fprintf(stderr, "consumed %" PRId64 "/%zu bytes.\n",
	    result->bit_length / 8, sz);
        return 0;
}
