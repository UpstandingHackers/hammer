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

#ifndef HAMMER_HAMMER__H
#define HAMMER_HAMMER__H

#include "compiler_specifics.h"

#ifndef HAMMER_INTERNAL__NO_STDARG_H
#include <stdarg.h>
#endif // HAMMER_INTERNAL__NO_STDARG_H
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include "allocator.h"

#define BYTE_BIG_ENDIAN 0x1
#define BIT_BIG_ENDIAN 0x2
#define BIT_LITTLE_ENDIAN 0x0
#define BYTE_LITTLE_ENDIAN 0x0

#ifdef __cplusplus
extern "C" {
#endif

typedef struct HParseState_ HParseState;

typedef enum HParserBackend_ {
  PB_MIN = 0,
  PB_PACKRAT = PB_MIN, // PB_MIN is always the default.
  PB_REGULAR,
  PB_LLk,
  PB_LALR,
  PB_GLR,
  PB_MAX = PB_GLR
} HParserBackend;

typedef enum HTokenType_ {
  // Before you change the explicit values of these, think of the poor bindings ;_;
  TT_INVALID = 0,
  TT_NONE = 1,
  TT_BYTES = 2,
  TT_SINT = 4,
  TT_UINT = 8,
  TT_SEQUENCE = 16,
  TT_RESERVED_1, // reserved for backend-specific internal use
  TT_ERR = 32,
  TT_USER = 64,
  TT_MAX
} HTokenType;

typedef struct HCountedArray_ {
  size_t capacity;
  size_t used;
  HArena * arena;
  struct HParsedToken_ **elements;
} HCountedArray;

typedef struct HBytes_ {
  const uint8_t *token;
  size_t len;
} HBytes;

#ifdef SWIG
typedef union {
  HBytes bytes;
  int64_t sint;
  uint64_t uint;
  double dbl;
  float flt;
  HCountedArray *seq;
  void *user;
} HTokenData;
#endif

typedef struct HParsedToken_ {
  HTokenType token_type;
#ifndef SWIG
  union {
    HBytes bytes;
    int64_t sint;
    uint64_t uint;
    double dbl;
    float flt;
    HCountedArray *seq; // a sequence of HParsedToken's
    void *user;
  };
#else
  HTokenData token_data;
#endif
  size_t index;
  size_t bit_length;
  char bit_offset;
} HParsedToken;

/**
 * The result of a successful parse. Note that this may reference the
 * input string.
 *
 * If a parse fails, the parse result will be NULL.
 * If a parse is successful but there's nothing there (i.e., if end_p 
 * succeeds) then there's a parse result but its ast is NULL.
 */
typedef struct HParseResult_ {
  const HParsedToken *ast;
  int64_t bit_length;
  HArena * arena;
} HParseResult;

/**
 * TODO: document me.
 * Relevant functions: h_bit_writer_new, h_bit_writer_put, h_bit_writer_get_buffer, h_bit_writer_free
 */
typedef struct HBitWriter_ HBitWriter;

typedef struct HCFChoice_ HCFChoice;
typedef struct HRVMProg_ HRVMProg;
typedef struct HParserVtable_ HParserVtable;

// TODO: Make this internal
typedef struct HParser_ {
  const HParserVtable *vtable;
  HParserBackend backend;
  void* backend_data;
  void *env;
  HCFChoice *desugared; /* if the parser can be desugared, its desugared form */
} HParser;

typedef struct HSuspendedParser_ HSuspendedParser;

/**
 * Type of an action to apply to an AST, used in the action() parser. 
 * It can be any (user-defined) function that takes a HParseResult*
 * and returns a HParsedToken*. (This is so that the user doesn't 
 * have to worry about memory allocation; action() does that for you.)
 * Note that the tagged union in HParsedToken* supports user-defined 
 * types, so you can create your own token types (corresponding to, 
 * say, structs) and stuff values for them into the void* in the 
 * tagged union in HParsedToken. 
 */
typedef HParsedToken* (*HAction)(const HParseResult *p, void* user_data);

/**
 * Type of a boolean attribute-checking function, used in the 
 * attr_bool() parser. It can be any (user-defined) function that takes
 * a HParseResult* and returns true or false. 
 */
typedef bool (*HPredicate)(HParseResult *p, void* user_data);

/**
 * Type of a parser that depends on the result of a previous parser,
 * used in h_bind(). The void* argument is passed through from h_bind() and can
 * be used to arbitrarily parameterize the function further.
 *
 * The HAllocator* argument gives access to temporary memory and is to be used
 * for any allocations inside the function. Specifically, construction of any
 * HParsers should use the '__m' combinator variants with the given allocator.
 * Anything allocated thus will be freed by 'h_bind'.
 */
typedef HParser* (*HContinuation)(HAllocator *mm__, const HParsedToken *x, void *env);

// {{{ Stuff for benchmarking
typedef struct HParserTestcase_ {
  unsigned char* input;
  size_t length;
  char* output_unambiguous;
} HParserTestcase;

#ifdef SWIG
typedef union {
  const char* actual_results;
  size_t parse_time;
} HResultTiming;
#endif

typedef struct HCaseResult_ {
  bool success;
#ifndef SWIG
  union {
    const char* actual_results; // on failure, filled in with the results of h_write_result_unamb
    size_t parse_time; // on success, filled in with time for a single parse, in nsec
  };
#else
  HResultTiming timestamp;
#endif
  size_t length;
} HCaseResult;

typedef struct HBackendResults_ {
  HParserBackend backend;
  bool compile_success;
  size_t n_testcases;
  size_t failed_testcases; // actually a count...
  HCaseResult *cases;
} HBackendResults;

typedef struct HBenchmarkResults_ {
  size_t len;
  HBackendResults *results;
} HBenchmarkResults;
// }}}

// {{{ Preprocessor definitions
#define HAMMER_FN_DECL_NOARG(rtype_t, name)		\
  rtype_t name(void);					\
  rtype_t name##__m(HAllocator* mm__)

#define HAMMER_FN_DECL(rtype_t, name, ...)		\
  rtype_t name(__VA_ARGS__);				\
  rtype_t name##__m(HAllocator* mm__, __VA_ARGS__)

#define HAMMER_FN_DECL_ATTR(attr, rtype_t, name, ...)			\
  rtype_t name(__VA_ARGS__) attr;					\
  rtype_t name##__m(HAllocator* mm__, __VA_ARGS__) attr

#ifndef SWIG
#define HAMMER_FN_DECL_VARARGS(rtype_t, name, ...)			\
  rtype_t name(__VA_ARGS__, ...);					\
  rtype_t name##__m(HAllocator* mm__, __VA_ARGS__, ...);		\
  rtype_t name##__mv(HAllocator* mm__, __VA_ARGS__, va_list ap);	\
  rtype_t name##__v(__VA_ARGS__, va_list ap);				\
  rtype_t name##__a(void *args[]);					\
  rtype_t name##__ma(HAllocator *mm__, void *args[])

// Note: this drops the attributes on the floor for the __v versions
#define HAMMER_FN_DECL_VARARGS_ATTR(attr, rtype_t, name, ...)		\
  rtype_t name(__VA_ARGS__, ...) attr;					\
  rtype_t name##__m(HAllocator* mm__, __VA_ARGS__, ...) attr;		\
  rtype_t name##__mv(HAllocator* mm__, __VA_ARGS__, va_list ap);	\
  rtype_t name##__v(__VA_ARGS__, va_list ap);				\
  rtype_t name##__a(void *args[]);					\
  rtype_t name##__ma(HAllocator *mm__, void *args[])
#else
#define HAMMER_FN_DECL_VARARGS(rtype_t, name, params...)  \
  rtype_t name(params, ...);				  \
  rtype_t name##__m(HAllocator* mm__, params, ...);    	  \
  rtype_t name##__a(void *args[]);			 \
  rtype_t name##__ma(HAllocator *mm__, void *args[])

// Note: this drops the attributes on the floor for the __v versions
#define HAMMER_FN_DECL_VARARGS_ATTR(attr, rtype_t, name, params...)		\
  rtype_t name(params, ...);				\
  rtype_t name##__m(HAllocator* mm__, params, ...);       	\
  rtype_t name##__a(void *args[]);				\
  rtype_t name##__ma(HAllocator *mm__, void *args[])
#endif // SWIG
// }}}


/**
 * Top-level function to call a parser that has been built over some
 * piece of input (of known size).
 */
HAMMER_FN_DECL(HParseResult*, h_parse, const HParser* parser, const uint8_t* input, size_t length);

/**
 * Initialize a parser for iteratively consuming an input stream in chunks.
 * This is only supported by some backends.
 *
 * Result is NULL if not supported by the backend.
 */
HAMMER_FN_DECL(HSuspendedParser*, h_parse_start, const HParser* parser);

/**
 * Run a suspended parser (as returned by h_parse_start) on a chunk of input.
 *
 * Returns true if the parser is done (needs no more input).
 */
bool h_parse_chunk(HSuspendedParser* s, const uint8_t* input, size_t length);

/**
 * Finish an iterative parse. Signals the end of input to the backend and
 * returns the parse result.
 */
HParseResult* h_parse_finish(HSuspendedParser* s);

/**
 * Given a string, returns a parser that parses that string value. 
 * 
 * Result token type: TT_BYTES
 */
HAMMER_FN_DECL(HParser*, h_token, const uint8_t *str, const size_t len);

#define h_literal(s) h_token(s, sizeof(s)-1)

/**
 * Given a single character, returns a parser that parses that 
 * character. 
 * 
 * Result token type: TT_UINT
 */
HAMMER_FN_DECL(HParser*, h_ch, const uint8_t c);

/**
 * Given two single-character bounds, lower and upper, returns a parser
 * that parses a single character within the range [lower, upper] 
 * (inclusive). 
 * 
 * Result token type: TT_UINT
 */
HAMMER_FN_DECL(HParser*, h_ch_range, const uint8_t lower, const uint8_t upper);

/**
 * Given an integer parser, p, and two integer bounds, lower and upper,
 * returns a parser that parses an integral value within the range 
 * [lower, upper] (inclusive).
 */
HAMMER_FN_DECL(HParser*, h_int_range, const HParser *p, const int64_t lower, const int64_t upper);

/**
 * Returns a parser that parses the specified number of bits. sign == 
 * true if signed, false if unsigned. 
 *
 * Result token type: TT_SINT if sign == true, TT_UINT if sign == false
 */
HAMMER_FN_DECL(HParser*, h_bits, size_t len, bool sign);

/**
 * Returns a parser that parses a signed 8-byte integer value. 
 *
 * Result token type: TT_SINT
 */
HAMMER_FN_DECL_NOARG(HParser*, h_int64);

/**
 * Returns a parser that parses a signed 4-byte integer value. 
 *
 * Result token type: TT_SINT
 */
HAMMER_FN_DECL_NOARG(HParser*, h_int32);

/**
 * Returns a parser that parses a signed 2-byte integer value. 
 *
 * Result token type: TT_SINT
 */
HAMMER_FN_DECL_NOARG(HParser*, h_int16);

/**
 * Returns a parser that parses a signed 1-byte integer value. 
 *
 * Result token type: TT_SINT
 */
HAMMER_FN_DECL_NOARG(HParser*, h_int8);

/**
 * Returns a parser that parses an unsigned 8-byte integer value. 
 *
 * Result token type: TT_UINT
 */
HAMMER_FN_DECL_NOARG(HParser*, h_uint64);

/**
 * Returns a parser that parses an unsigned 4-byte integer value. 
 *
 * Result token type: TT_UINT
 */
HAMMER_FN_DECL_NOARG(HParser*, h_uint32);

/**
 * Returns a parser that parses an unsigned 2-byte integer value. 
 *
 * Result token type: TT_UINT
 */
HAMMER_FN_DECL_NOARG(HParser*, h_uint16);

/**
 * Returns a parser that parses an unsigned 1-byte integer value. 
 *
 * Result token type: TT_UINT
 */
HAMMER_FN_DECL_NOARG(HParser*, h_uint8);

/**
 * Given another parser, p, returns a parser that skips any whitespace 
 * and then applies p. 
 *
 * Result token type: p's result type
 */
HAMMER_FN_DECL(HParser*, h_whitespace, const HParser* p);

/**
 * Given two parsers, p and q, returns a parser that parses them in
 * sequence but only returns p's result.
 *
 * Result token type: p's result type
 */
HAMMER_FN_DECL(HParser*, h_left, const HParser* p, const HParser* q);

/**
 * Given two parsers, p and q, returns a parser that parses them in
 * sequence but only returns q's result.
 *
 * Result token type: q's result type
 */
HAMMER_FN_DECL(HParser*, h_right, const HParser* p, const HParser* q);

/**
 * Given three parsers, p, x, and q, returns a parser that parses them in
 * sequence but only returns x's result.
 *
 * Result token type: x's result type
 */
HAMMER_FN_DECL(HParser*, h_middle, const HParser* p, const HParser* x, const HParser* q);

/**
 * Given another parser, p, and a function f, returns a parser that 
 * applies p, then applies f to everything in the AST of p's result. 
 *
 * Result token type: any
 */
HAMMER_FN_DECL(HParser*, h_action, const HParser* p, const HAction a, void* user_data);

/**
 * Parse a single character in the given charset. 
 *
 * Result token type: TT_UINT
 */
HAMMER_FN_DECL(HParser*, h_in, const uint8_t *charset, size_t length);

/**
 * Parse a single character *NOT* in the given charset. 
 *
 * Result token type: TT_UINT
 */
HAMMER_FN_DECL(HParser*, h_not_in, const uint8_t *charset, size_t length);

/**
 * A no-argument parser that succeeds if there is no more input to 
 * parse. 
 *
 * Result token type: None. The HParseResult exists but its AST is NULL.
 */
HAMMER_FN_DECL_NOARG(HParser*, h_end_p);

/**
 * This parser always fails. 
 *
 * Result token type: NULL. Always.
 */
HAMMER_FN_DECL_NOARG(HParser*, h_nothing_p);

/**
 * Given a null-terminated list of parsers, apply each parser in order.
 * The parse succeeds only if all parsers succeed. 
 *
 * Result token type: TT_SEQUENCE
 */
HAMMER_FN_DECL_VARARGS_ATTR(H_GCC_ATTRIBUTE((sentinel)), HParser*, h_sequence, HParser* p);

/**
 * Given an array of parsers, p_array, apply each parser in order. The 
 * first parser to succeed is the result; if no parsers succeed, the 
 * parse fails. 
 *
 * Result token type: The type of the first successful parser's result.
 */
HAMMER_FN_DECL_VARARGS_ATTR(H_GCC_ATTRIBUTE((sentinel)), HParser*, h_choice, HParser* p);

/**
 * Given a null-terminated list of parsers, match a permutation phrase of these
 * parsers, i.e. match all parsers exactly once in any order.
 *
 * If multiple orders would match, the lexically smallest permutation is used;
 * in other words, at any step the remaining available parsers are tried in
 * the order in which they appear in the arguments.
 * 
 * As an exception, 'h_optional' parsers (actually those that return a result
 * of token type TT_NONE) are detected and the algorithm will try to match them
 * with a non-empty result. Specifically, a result of TT_NONE is treated as a
 * non-match as long as any other argument matches.
 *
 * Other parsers that succeed on any input (e.g. h_many), that match the same
 * input as others, or that match input which is a prefix of another match can
 * lead to unexpected results and should probably not be used as arguments.
 *
 * The result is a sequence of the same length as the argument list.
 * Each parser's result is placed at that parser's index in the arguments.
 * The permutation itself (the order in which the arguments were matched) is
 * not returned.
 *
 * Result token type: TT_SEQUENCE
 */
HAMMER_FN_DECL_VARARGS_ATTR(H_GCC_ATTRIBUTE((sentinel)), HParser*, h_permutation, HParser* p);

/**
 * Given two parsers, p1 and p2, this parser succeeds in the following 
 * cases: 
 * - if p1 succeeds and p2 fails
 * - if both succeed but p1's result is as long as or longer than p2's
 *
 * Result token type: p1's result type.
 */
HAMMER_FN_DECL(HParser*, h_butnot, const HParser* p1, const HParser* p2);

/**
 * Given two parsers, p1 and p2, this parser succeeds in the following 
 * cases:
 * - if p1 succeeds and p2 fails
 * - if both succeed but p2's result is shorter than p1's
 *
 * Result token type: p1's result type.
 */
HAMMER_FN_DECL(HParser*, h_difference, const HParser* p1, const HParser* p2);

/**
 * Given two parsers, p1 and p2, this parser succeeds if *either* p1 or
 * p2 succeed, but not if they both do.
 *
 * Result token type: The type of the result of whichever parser succeeded.
 */
HAMMER_FN_DECL(HParser*, h_xor, const HParser* p1, const HParser* p2);

/**
 * Given a parser, p, this parser succeeds for zero or more repetitions
 * of p. 
 *
 * Result token type: TT_SEQUENCE
 */
HAMMER_FN_DECL(HParser*, h_many, const HParser* p);

/**
 * Given a parser, p, this parser succeeds for one or more repetitions 
 * of p. 
 *
 * Result token type: TT_SEQUENCE
 */
HAMMER_FN_DECL(HParser*, h_many1, const HParser* p);

/**
 * Given a parser, p, this parser succeeds for exactly N repetitions 
 * of p. 
 *
 * Result token type: TT_SEQUENCE
 */
HAMMER_FN_DECL(HParser*, h_repeat_n, const HParser* p, const size_t n);

/**
 * Given a parser, p, this parser succeeds with the value p parsed or 
 * with an empty result. 
 *
 * Result token type: If p succeeded, the type of its result; if not, TT_NONE.
 */
HAMMER_FN_DECL(HParser*, h_optional, const HParser* p);

/**
 * Given a parser, p, this parser succeeds if p succeeds, but doesn't 
 * include p's result in the result. 
 *
 * Result token type: None. The HParseResult exists but its AST is NULL.
 */
HAMMER_FN_DECL(HParser*, h_ignore, const HParser* p);

/**
 * Given a parser, p, and a parser for a separator, sep, this parser 
 * matches a (possibly empty) list of things that p can parse, 
 * separated by sep.
 * For example, if p is repeat1(range('0','9')) and sep is ch(','), 
 * sepBy(p, sep) will match a comma-separated list of integers. 
 *
 * Result token type: TT_SEQUENCE
 */
HAMMER_FN_DECL(HParser*, h_sepBy, const HParser* p, const HParser* sep);

/**
 * Given a parser, p, and a parser for a separator, sep, this parser matches a list of things that p can parse, separated by sep. Unlike sepBy, this ensures that the result has at least one element.
 * For example, if p is repeat1(range('0','9')) and sep is ch(','), sepBy1(p, sep) will match a comma-separated list of integers. 
 *
 * Result token type: TT_SEQUENCE
 */
HAMMER_FN_DECL(HParser*, h_sepBy1, const HParser* p, const HParser* sep);

/**
 * This parser always returns a zero length match, i.e., empty string. 
 *
 * Result token type: None. The HParseResult exists but its AST is NULL.
 */
HAMMER_FN_DECL_NOARG(HParser*, h_epsilon_p);

/**
 * This parser applies its first argument to read an unsigned integer
 * value, then applies its second argument that many times. length 
 * should parse an unsigned integer value; this is checked at runtime.
 * Specifically, the token_type of the returned token must be TT_UINT.
 * In future we might relax this to include TT_USER but don't count on it.
 *
 * Result token type: TT_SEQUENCE
 */
HAMMER_FN_DECL(HParser*, h_length_value, const HParser* length, const HParser* value);

/**
 * This parser attaches a predicate function, which returns true or 
 * false, to a parser. The function is evaluated over the parser's 
 * result. 
 *
 * The parse only succeeds if the attribute function returns true. 
 *
 * attr_bool will check whether p's result exists and whether p's 
 * result AST exists; you do not need to check for this in your 
 * predicate function.
 * 
 * Result token type: p's result type if pred succeeded, NULL otherwise.
 */
HAMMER_FN_DECL(HParser*, h_attr_bool, const HParser* p, HPredicate pred, void* user_data);

/**
 * The 'and' parser asserts that a conditional syntax is satisfied, 
 * but doesn't consume that conditional syntax. 
 * This is useful for lookahead. As an example:
 *
 * Suppose you already have a parser, hex_p, that parses numbers in 
 * hexadecimal format (including the leading '0x'). Then
 *   sequence(and(token((const uint8_t*)"0x", 2)), hex_p)
 * checks to see whether there is a leading "0x", *does not* consume 
 * the "0x", and then applies hex_p to parse the hex-formatted number.
 *
 * 'and' succeeds if p succeeds, and fails if p fails. 
 *
 * Result token type: None. The HParseResult exists but its AST is NULL.
 */
HAMMER_FN_DECL(HParser*, h_and, const HParser* p);

/**
 * The 'not' parser asserts that a conditional syntax is *not* 
 * satisfied, but doesn't consume that conditional syntax.
 * As a somewhat contrived example:
 * 
 * Since 'choice' applies its arguments in order, the following parser:
 *   sequence(ch('a'), choice(ch('+'), token((const uint8_t*)"++"), NULL), ch('b'), NULL)
 * will not parse "a++b", because once choice() has succeeded, it will 
 * not backtrack and try other alternatives if a later parser in the 
 * sequence fails. 
 * Instead, you can force the use of the second alternative by turning 
 * the ch('+') alternative into a sequence with not:
 *   sequence(ch('a'), choice(sequence(ch('+'), not(ch('+')), NULL), token((const uint8_t*)"++")), ch('b'), NULL)
 * If the input string is "a+b", the first alternative is applied; if 
 * the input string is "a++b", the second alternative is applied.
 * 
 * Result token type: None. The HParseResult exists but its AST is NULL.
 */
HAMMER_FN_DECL(HParser*, h_not, const HParser* p);

/**
 * Create a parser that just calls out to another, as yet unknown, 
 * parser.
 * Note that the inner parser gets bound later, with bind_indirect.
 * This can be used to create recursive parsers.
 *
 * Result token type: the type of whatever parser is bound to it with
 * bind_indirect().
 */
HAMMER_FN_DECL_NOARG(HParser*, h_indirect);

/**
 * Set the inner parser of an indirect. See comments on indirect for 
 * details.
 */
HAMMER_FN_DECL(void, h_bind_indirect, HParser* indirect, const HParser* inner);

/**
 * This parser runs its argument parser with the given endianness setting.
 *
 * The value of 'endianness' should be a bit-wise or of the constants
 * BYTE_BIG_ENDIAN/BYTE_LITTLE_ENDIAN and BIT_BIG_ENDIAN/BIT_LITTLE_ENDIAN.
 *
 * Result token type: p's result type.
 */
HAMMER_FN_DECL(HParser*, h_with_endianness, char endianness, const HParser* p);

/**
 * The 'h_put_value' combinator stashes the result of the parser
 * it wraps in a symbol table in the parse state, so that non-
 * local actions and predicates can access this value. 
 *
 * Try not to use this combinator if you can avoid it. 
 *
 * Result token type: p's token type if name was not already in
 * the symbol table. It is an error, and thus a NULL result (and
 * parse failure), to attempt to rename a symbol.
 */
HAMMER_FN_DECL(HParser*, h_put_value, const HParser *p, const char* name);

/**
 * The 'h_get_value' combinator retrieves a named HParseResult that
 * was previously stashed in the parse state. 
 * 
 * Try not to use this combinator if you can avoid it.
 * 
 * Result token type: whatever the stashed HParseResult is, if
 * present. If absent, NULL (and thus parse failure).
 */
HAMMER_FN_DECL(HParser*, h_get_value, const char* name);

/**
 * Monadic bind for HParsers, i.e.:
 * Sequencing where later parsers may depend on the result(s) of earlier ones.
 *
 * Run p and call the result x. Then run k(env,x).  Fail if p fails or if
 * k(env,x) fails or if k(env,x) is NULL.
 *
 * Result: the result of k(x,env).
 */
HAMMER_FN_DECL(HParser*, h_bind, const HParser *p, HContinuation k, void *env);

/**
 * Free the memory allocated to an HParseResult when it is no longer needed.
 */
HAMMER_FN_DECL(void, h_parse_result_free, HParseResult *result);

// Some debugging aids
/**
 * Format token into a compact unambiguous form. Useful for parser test cases.
 * Caller is responsible for freeing the result.
 */
char* h_write_result_unamb(const HParsedToken* tok);
/**
 * Format token to the given output stream. Indent starting at [indent] spaces,
 * with [delta] spaces between levels.
 *
 * Note: This function does not print a trailing newline. It also does not
 * print any spaces to indent the initial line of output. This makes it
 * suitable for recursive use in the condensed output of larger structures.
 */
void h_pprint(FILE* stream, const HParsedToken* tok, int indent, int delta);
/**
 * Format token to the given output. Print a trailing newline.
 *
 * This function assumes an initial indentation of 0 and uses 2 spaces between
 * indentation levels. It is equivalent to 'h_pprint(stream, tok, 0, 2)'
 * followed by 'fputc('\n', stream)' and is provided for convenience.
 */
void h_pprintln(FILE* stream, const HParsedToken* tok);

/**
 * Build parse tables for the given parser backend. See the
 * documentation for the parser backend in question for information
 * about the [params] parameter, or just pass in NULL for the defaults.
 *
 * Returns -1 if grammar cannot be compiled with the specified options; 0 otherwise.
 */
HAMMER_FN_DECL(int, h_compile, HParser* parser, HParserBackend backend, const void* params);

/**
 * TODO: Document me
 */
HBitWriter *h_bit_writer_new(HAllocator* mm__);

/**
 * TODO: Document me
 */
void h_bit_writer_put(HBitWriter* w, uint64_t data, size_t nbits);

/**
 * TODO: Document me
 * Must not free [w] until you're done with the result.
 * [len] is in bytes.
 */
const uint8_t* h_bit_writer_get_buffer(HBitWriter* w, size_t *len);

/**
 * TODO: Document me
 */
void h_bit_writer_free(HBitWriter* w);

// General-purpose actions for use with h_action
// XXX to be consolidated with glue.h when merged upstream
HParsedToken *h_act_first(const HParseResult *p, void* userdata);
HParsedToken *h_act_second(const HParseResult *p, void* userdata);
HParsedToken *h_act_last(const HParseResult *p, void* userdata);
HParsedToken *h_act_flatten(const HParseResult *p, void* userdata);
HParsedToken *h_act_ignore(const HParseResult *p, void* userdata);

// {{{ Benchmark functions
HAMMER_FN_DECL(HBenchmarkResults *, h_benchmark, HParser* parser, HParserTestcase* testcases);
void h_benchmark_report(FILE* stream, HBenchmarkResults* results);
//void h_benchmark_dump_optimized_code(FILE* stream, HBenchmarkResults* results);
// }}}

// {{{ result_buf printers (used by token type registry)

struct result_buf;

bool h_append_buf(struct result_buf *buf, const char* input, int len);
bool h_append_buf_c(struct result_buf *buf, char v);
bool h_append_buf_formatted(struct result_buf *buf, char* format, ...);

// }}}

// {{{ Token type registry
/// Allocate a new, unused (as far as this function knows) token type.
HTokenType h_allocate_token_type(const char* name);

/// Allocate a new token type with an unambiguous print function.
HTokenType h_allocate_token_new(
    const char* name,
    void (*unamb_sub)(const HParsedToken *tok, struct result_buf *buf),
    void (*pprint)(FILE* stream, const HParsedToken* tok, int indent, int delta));

/// Get the token type associated with name. Returns -1 if name is unkown
HTokenType h_get_token_type_number(const char* name);

/// Get the name associated with token_type. Returns NULL if the token type is unkown
const char* h_get_token_type_name(HTokenType token_type);
// }}}

/// Make an allocator that draws from the given memory area.
HAllocator *h_sloballoc(void *mem, size_t size);

#ifdef __cplusplus
}
#endif

#endif // #ifndef HAMMER_HAMMER__H
