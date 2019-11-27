//
// API additions for writing grammar and semantic actions more concisely
//
//
// Quick Overview:
//
// Grammars can be succinctly specified with the family of H_RULE macros.
// H_RULE defines a plain parser variable. H_ARULE additionally attaches a
// semantic action; H_VRULE attaches a validation. H_AVRULE and H_VARULE
// combine both.
//
// A few standard semantic actions are defined below. The H_ACT_APPLY macro
// allows semantic actions to be defined by "partial application" of
// a generic action to fixed paramters. H_VALIDATE_APPLY is similar for
// h_atter_bool.
//
// The definition of more complex semantic actions will usually consist of
// extracting data from the given parse tree and constructing a token of custom
// type to represent the result. A number of functions and convenience macros
// are provided to capture the most common cases and idioms.
//
// See the leading comment blocks on the sections below for more details.
//

#ifndef HAMMER_GLUE__H
#define HAMMER_GLUE__H

#include <assert.h>
#include "hammer.h"


//
// Grammar specification
//
// H_RULE is simply a short-hand for the typical declaration and definition of
// a parser variable. See its plain definition below. The goal is to save
// horizontal space as well as to provide a clear and unified look together with
// the other macro variants that stays close to an abstract PEG or BNF grammar.
// The latter goal is more specifically enabled by H_ARULE, H_VRULE, and their
// combinations as they allow the definition of syntax to be given without
// intermingling it with the semantic specifications.
//
// H_ARULE defines a variable just like H_RULE but attaches a semantic action
// to the result of the parser via h_action. The action is expected to be
// named act_<rulename>.
//
// H_VRULE is analogous to H_ARULE but attaches a validation via h_attr_bool.
// The validation is expected to be named validate_<rulename>.
//
// H_VARULE combines H_RULE with both an action and a validation. The action is
// attached before the validation, i.e. the validation receives as input the
// result of the action.
//
// H_AVRULE is like H_VARULE but the action is attached outside the validation,
// i.e. the validation receives the uninterpreted AST as input.
//
// H_ADRULE, H_VDRULE, H_AVDRULE, and H_VADRULE are the same as the
// equivalent non-D variants, except that they also allow you to uset
// the user_data pointer.  In cases where both an attr_bool and an
// action are used, the same userdata pointer is given to both.

#define H_RULE(rule, def)  HParser *rule = def
#define H_ARULE(rule, def) HParser *rule = h_action(def, act_ ## rule, NULL)
#define H_VRULE(rule, def) HParser *rule = \
    h_attr_bool(def, validate_ ## rule, NULL)
#define H_VARULE(rule, def) HParser *rule = \
    h_attr_bool(h_action(def, act_ ## rule, NULL), validate_ ## rule, NULL)
#define H_AVRULE(rule, def) HParser *rule = \
    h_action(h_attr_bool(def, validate_ ## rule, NULL), act_ ## rule, NULL)
#define H_ADRULE(rule, def, data) HParser *rule =       \
    h_action(def, act_ ## rule, data)
#define H_VDRULE(rule, def, data) HParser *rule =       \
    h_attr_bool(def, validate_ ## rule, data)
#define H_VADRULE(rule, def, data) HParser *rule =              \
    h_attr_bool(h_action(def, act_ ## rule, data), validate_ ## rule, data)
#define H_AVDRULE(rule, def, data) HParser *rule =              \
    h_action(h_attr_bool(def, validate_ ## rule, data), act_ ## rule, data)


//
// Pre-fab semantic actions
//
// A collection of generally useful semantic actions is provided.
//
// h_act_ignore is the action equivalent of the parser combinator h_ignore. It
// simply causes the AST it is applied to to be replaced with NULL. This most
// importantly causes it to be elided from the result of a surrounding
// h_sequence.
//
// h_act_index is of note as it is not itself suitable to be passed to
// h_action. It is parameterized by an index to be picked from a sequence
// token. It must be wrapped in a proper HAction to be used. The H_ACT_APPLY
// macro provides a concise way to define such a parameter-application wrapper.
//
// h_act_flatten acts on a token of possibly nested sequences by recursively
// flattening it into a single sequence. Cf. h_seq_flatten below.
//
// H_ACT_APPLY implements "partial application" for semantic actions. It
// defines a new action that supplies given parameters to a parameterized
// action such as h_act_index.
//

HParsedToken *h_act_index(int i, const HParseResult *p, void* user_data);
HParsedToken *h_act_first(const HParseResult *p, void* user_data);
HParsedToken *h_act_second(const HParseResult *p, void* user_data);
HParsedToken *h_act_last(const HParseResult *p, void* user_data);
HParsedToken *h_act_flatten(const HParseResult *p, void* user_data);
HParsedToken *h_act_ignore(const HParseResult *p, void* user_data);

// Define 'myaction' as a specialization of 'paction' by supplying the leading
// parameters.
#define H_ACT_APPLY(myaction, paction, ...) \
  HParsedToken *myaction(const HParseResult *p, void* user_data) {      \
    return paction(__VA_ARGS__, p, user_data);                          \
  }

// Similar, but for validations.
#define H_VALIDATE_APPLY(myvalidation, pvalidation, ...)  \
  bool myvalidation(HParseResult* p, void* user_data) {   \
    return pvalidation(__VA_ARGS__, p, user_data);        \
  }


//
// Working with HParsedTokens
//
// The type HParsedToken represents a dynamically-typed universe of values.
// Declared below are constructors to turn ordinary values into their
// HParsedToken equivalents, extractors to retrieve the original values from
// inside an HParsedToken, and functions that inspect and modify tokens of
// sequence type directly.
//
// In addition, there are a number of short-hand macros that work with some
// conventions to eliminate common boilerplate. These conventions are listed
// below. Be sure to follow them if you want to use the respective macros.
//
//  * The single argument to semantic actions should be called 'p'.
//
//    The H_MAKE macros suppy 'p->arena' to their underlying h_make
//    counterparts. The H_FIELD macros supply 'p->ast' to their underlying
//    H_INDEX counterparts.
//
//  * For each custom token type, there should be a typedef for the
//    corresponding value type.
//
//    H_CAST, H_INDEX and H_FIELD cast the void * user field of such a token to
//    a pointer to the given type. 
//
//  * For each custom token type, say 'foo_t', there must be an integer
//    constant 'TT_foo_t' to identify the token type. This constant must have a
//    value greater or equal than TT_USER.
//
//    One idiom is to define an enum for all custom token types and to assign a
//    value of TT_USER to the first element. This can be viewed as extending
//    the HTokenType enum.
//
//    The H_MAKE and H_ASSERT macros derive the name of the token type constant
//    from the given type name.
//
//
// The H_ALLOC macro is useful for allocating values of custom token types.
//
// The H_MAKE family of macros construct tokens of a given type. The native
// token types are indicated by a corresponding suffix such as in H_MAKE_SEQ.
// The form with no suffix is used for custom token types. This convention is
// also used for other macro and function families.
//
// The H_ASSERT family simply asserts that a given token has the expected type.
// It mainly serves as an implementation aid for H_CAST. Of note in that regard
// is that, unlike the standard 'assert' macro, these form _expressions_ that
// return the value of their token argument; thus they can be used in a
// "pass-through" fashion inside other expressions.
//
// The H_CAST family combines a type assertion with access to the
// statically-typed value inside a token.
//
// A number of functions h_seq_* operate on and inspect sequence tokens.
// Note that H_MAKE_SEQ takes no arguments and constructs an empty sequence.
// Therefore there are h_seq_snoc and h_seq_append to build up sequences.
//
// The macro families H_FIELD and H_INDEX combine index access on a sequence
// with a cast to the appropriate result type. H_FIELD is used to access the
// elements of the argument token 'p' in an action. H_INDEX allows any sequence
// token to be specified. Both macro families take an arbitrary number of index
// arguments, giving access to elements in nested sequences by path.
// These macros are very useful to avoid spaghetti chains of unchecked pointer
// dereferences.
//

// Standard short-hand for arena-allocating a variable in a semantic action.
#define H_ALLOC(TYP)  ((TYP *) h_arena_malloc(p->arena, sizeof(TYP)))

// Token constructors...

HParsedToken *h_make(HArena *arena, HTokenType type, void *value);
HParsedToken *h_make_seq(HArena *arena);  // Makes empty sequence.
HParsedToken *h_make_seqn(HArena *arena, size_t n);  // Makes empty sequence of expected size n.
HParsedToken *h_make_bytes(HArena *arena, const uint8_t *array, size_t len);
HParsedToken *h_make_sint(HArena *arena, int64_t val);
HParsedToken *h_make_uint(HArena *arena, uint64_t val);

// Standard short-hands to make tokens in an action.
#define H_MAKE(TYP, VAL)  h_make(p->arena, (HTokenType)TT_ ## TYP, VAL)
#define H_MAKE_SEQ()      h_make_seq(p->arena)
#define H_MAKE_SEQN(N)    h_make_seqn(p->arena, N)
#define H_MAKE_BYTES(VAL, LEN) h_make_bytes(p->arena, VAL, LEN)
#define H_MAKE_SINT(VAL)  h_make_sint(p->arena, VAL)
#define H_MAKE_UINT(VAL)  h_make_uint(p->arena, VAL)

// Extract (cast) type-specific value back from HParsedTokens...

// Pass-through assertion that a given token has the expected type.
#define h_assert_type(T,P)  (assert(P->token_type == (HTokenType)T), P)

// Convenience short-hand forms of h_assert_type.
#define H_ASSERT(TYP, TOK)   h_assert_type(TT_ ## TYP, TOK)
#define H_ASSERT_SEQ(TOK)    h_assert_type(TT_SEQUENCE, TOK)
#define H_ASSERT_BYTES(TOK)  h_assert_type(TT_BYTES, TOK)
#define H_ASSERT_SINT(TOK)   h_assert_type(TT_SINT, TOK)
#define H_ASSERT_UINT(TOK)   h_assert_type(TT_UINT, TOK)

// Assert expected type and return contained value.
#define H_CAST(TYP, TOK)   ((TYP *) H_ASSERT(TYP, TOK)->user)
#define H_CAST_SEQ(TOK)    (H_ASSERT_SEQ(TOK)->seq)
#define H_CAST_BYTES(TOK)  (H_ASSERT_BYTES(TOK)->bytes)
#define H_CAST_SINT(TOK)   (H_ASSERT_SINT(TOK)->sint)
#define H_CAST_UINT(TOK)   (H_ASSERT_UINT(TOK)->uint)

// Sequence access...

// Return the length of a sequence.
size_t h_seq_len(const HParsedToken *p);

// Access a sequence's element array.
HParsedToken **h_seq_elements(const HParsedToken *p);

// Access a sequence element by index.
HParsedToken *h_seq_index(const HParsedToken *p, size_t i);

// Access an element in a nested sequence by a path of indices.
HParsedToken *h_seq_index_path(const HParsedToken *p, size_t i, ...);
HParsedToken *h_seq_index_vpath(const HParsedToken *p, size_t i, va_list va);

// Convenience macros combining (nested) index access and h_cast.
#define H_INDEX(TYP, SEQ, ...)   H_CAST(TYP, H_INDEX_TOKEN(SEQ, __VA_ARGS__))
#define H_INDEX_SEQ(SEQ, ...)    H_CAST_SEQ(H_INDEX_TOKEN(SEQ, __VA_ARGS__))
#define H_INDEX_BYTES(SEQ, ...)  H_CAST_BYTES(H_INDEX_TOKEN(SEQ, __VA_ARGS__))
#define H_INDEX_SINT(SEQ, ...)   H_CAST_SINT(H_INDEX_TOKEN(SEQ, __VA_ARGS__))
#define H_INDEX_UINT(SEQ, ...)   H_CAST_UINT(H_INDEX_TOKEN(SEQ, __VA_ARGS__))
#define H_INDEX_TOKEN(SEQ, ...)  h_seq_index_path(SEQ, __VA_ARGS__, -1)

// Standard short-hand to access and cast elements on a sequence token.
#define H_FIELD(TYP, ...)  H_INDEX(TYP, p->ast, __VA_ARGS__)
#define H_FIELD_SEQ(...)   H_INDEX_SEQ(p->ast, __VA_ARGS__)
#define H_FIELD_BYTES(...) H_INDEX_BYTES(p->ast, __VA_ARGS__)
#define H_FIELD_SINT(...)  H_INDEX_SINT(p->ast, __VA_ARGS__)
#define H_FIELD_UINT(...)  H_INDEX_UINT(p->ast, __VA_ARGS__)

// Lower-level helper for h_seq_index.
HParsedToken *h_carray_index(const HCountedArray *a, size_t i); // XXX -> internal

// Sequence modification...

// Add elements to a sequence.
void h_seq_snoc(HParsedToken *xs, const HParsedToken *x);     // append one
void h_seq_append(HParsedToken *xs, const HParsedToken *ys);  // append many

// XXX TODO: Remove elements from a sequence.

// Flatten nested sequences into one.
const HParsedToken *h_seq_flatten(HArena *arena, const HParsedToken *p);


#endif
