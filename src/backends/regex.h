// Internal defs
#ifndef HAMMER_BACKEND_REGEX__H
#define HAMMER_BACKEND_REGEX__H

// each insn is an 8-bit opcode and a 16-bit parameter
// [a] are actions; they add an instruction to the stackvm that is being output.
// [m] are match ops; they can either succeed or fail, depending on the current character
// [c] are control ops. They affect the pc non-linearly.
typedef enum HRVMOp_ {
  RVM_ACCEPT,  // [a]
  RVM_GOTO,    // [c] parameter is an offset into the instruction table
  RVM_FORK,    // [c] parameter is an offset into the instruction table
  RVM_PUSH,    // [a] No arguments, just pushes a mark (pointer to some
               //     character in the input string) onto the stack
  RVM_ACTION,  // [a] argument is an action ID
  RVM_CAPTURE, // [a] Capture the last string (up to the current
	       //     position, non-inclusive), and push it on the
	       //     stack. No arg.
  RVM_EOF,     // [m] Succeeds only if at EOF.
  RVM_MATCH,   // [m] The high byte of the parameter is an upper bound
	       //     and the low byte is a lower bound, both
	       //     inclusive. An inverted match should be handled
	       //     as two ranges.
  RVM_STEP,    // [a] Step to the next byte of input
  RVM_OPCOUNT
} HRVMOp;

typedef struct HRVMInsn_{
  uint8_t op;
  uint16_t arg;
} HRVMInsn;

#define TT_MARK TT_RESERVED_1

typedef struct HSVMContext_ {
  HParsedToken **stack;
  size_t stack_count; // number of items on the stack. Thus stack[stack_count] is the first unused item on the stack.
  size_t stack_capacity;
} HSVMContext;

// These actions all assume that the items on the stack are not
// aliased anywhere.
typedef bool (*HSVMActionFunc)(HArena *arena, HSVMContext *ctx, void* env);
typedef struct HSVMAction_ {
  HSVMActionFunc action;
  void* env;
} HSVMAction;

struct HRVMProg_ {
  HAllocator *allocator;
  size_t length;
  size_t action_count;
  HRVMInsn *insns;
  HSVMAction *actions;
};

// Returns true IFF the provided parser could be compiled.
bool h_compile_regex(HRVMProg *prog, const HParser* parser);

// These functions are used by the compile_to_rvm method of HParser
uint16_t h_rvm_create_action(HRVMProg *prog, HSVMActionFunc action_func, void* env);

// returns the address of the instruction just created
uint16_t h_rvm_insert_insn(HRVMProg *prog, HRVMOp op, uint16_t arg);

// returns the address of the next insn to be created.
uint16_t h_rvm_get_ip(HRVMProg *prog);

// Used to insert forward references; the idea is to generate a JUMP
// or FORK instruction with a target of 0, then update it once the
// correct target is known.
void h_rvm_patch_arg(HRVMProg *prog, uint16_t ip, uint16_t new_val);

// Common SVM action funcs...
bool h_svm_action_make_sequence(HArena *arena, HSVMContext *ctx, void* env);
bool h_svm_action_clear_to_mark(HArena *arena, HSVMContext *ctx, void* env);

extern HParserBackendVTable h__regex_backend_vtable;

#endif
