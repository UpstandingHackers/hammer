#include <string.h>
#include <assert.h>
#include "../internal.h"
#include "../parsers/parser_internal.h"
#include "regex.h"

#undef a_new
#define a_new(typ, count) a_new_(arena, typ, count)
// Stack VM
typedef enum HSVMOp_ {
  SVM_PUSH, // Push a mark. There is no VM insn to push an object.
  SVM_NOP, // Used to start the chain, and possibly elsewhere. Does nothing.
  SVM_ACTION, // Same meaning as RVM_ACTION
  SVM_CAPTURE, // Same meaning as RVM_CAPTURE
  SVM_ACCEPT,
} HSVMOp;

typedef struct HRVMTrace_ {
  struct HRVMTrace_ *next; // When parsing, these are
			   // reverse-threaded. There is a postproc
			   // step that inverts all the pointers.
  size_t input_pos;
  uint16_t arg;
  uint8_t opcode;
} HRVMTrace;

typedef struct HRVMThread_ {
  HRVMTrace *trace;
  uint16_t ip;
} HRVMThread;

HParseResult *run_trace(HAllocator *mm__, HRVMProg *orig_prog, HRVMTrace *trace, const uint8_t *input, int len);

HRVMTrace *invert_trace(HRVMTrace *trace) {
  HRVMTrace *last = NULL;
  if (!trace)
    return NULL;
  if (!trace->next)
    return trace;
  do {
    HRVMTrace *next = trace->next;
    trace->next = last;
    last = trace;
    trace = next;
  } while (trace->next);
  return trace;
}

void* h_rvm_run__m(HAllocator *mm__, HRVMProg *prog, const uint8_t* input, size_t len) {
  HArena *arena = h_new_arena(mm__, 0);
  HRVMTrace **heads_p = a_new(HRVMTrace*, prog->length),
    **heads_n = a_new(HRVMTrace*, prog->length);

  HRVMTrace *ret_trace;
  
  uint8_t *insn_seen = a_new(uint8_t, prog->length); // 0 -> not seen, 1->processed, 2->queued
  HRVMThread *ip_queue = a_new(HRVMThread, prog->length);
  size_t ipq_top;

  
  

#define THREAD ip_queue[ipq_top-1]
#define PUSH_SVM(op_, arg_) do { \
	  HRVMTrace *nt = a_new(HRVMTrace, 1); \
	  nt->arg = (arg_);		       \
	  nt->opcode = (op_);		       \
	  nt->next = THREAD.trace;	       \
	  nt->input_pos = off;		       \
	  THREAD.trace = nt;		       \
  } while(0)
    
  heads_n[0] = a_new(HRVMTrace, 1); // zeroing
  heads_n[0]->opcode = SVM_NOP;

  size_t off = 0;
  int live_threads = 1;
  for (off = 0; off <= len; off++) {
    uint8_t ch = ((off == len) ? 0 : input[off]);
    size_t ip_s; // BUG: there was an unused variable ip. Not sure if
		 // I intended to use it somewhere.
    /* scope */ {
      HRVMTrace **heads_t;
      heads_t = heads_n;
      heads_n = heads_p;
      heads_p = heads_t;
      memset(heads_n, 0, prog->length * sizeof(*heads_n));
    }
    memset(insn_seen, 0, prog->length); // no insns seen yet
    if (!live_threads)
      goto match_fail;
    live_threads = 0;
    for (ip_s = 0; ip_s < prog->length; ip_s++) {
      ipq_top = 1;
      // TODO: Write this as a threaded VM
      if (!heads_p[ip_s])
	continue;
      THREAD.ip = ip_s;

      uint8_t hi, lo;
      uint16_t arg;
      while(ipq_top > 0) {
	if (insn_seen[THREAD.ip] == 1)
	  continue;
	insn_seen[THREAD.ip] = 1;
	arg = prog->insns[THREAD.ip].arg;
	switch(prog->insns[THREAD.ip].op) {
	case RVM_ACCEPT:
	  PUSH_SVM(SVM_ACCEPT, 0);
	  ret_trace = THREAD.trace;
	  goto run_trace;
	case RVM_MATCH:
	  // Doesn't actually validate the "must be followed by MATCH
	  // or STEP. It should. Preproc perhaps?
	  hi = (arg >> 8) & 0xff;
	  lo = arg & 0xff;
	  THREAD.ip++;
	  if (ch < lo || ch > hi)
	    ipq_top--; // terminate thread
	  goto next_insn;
	case RVM_GOTO:
	  THREAD.ip = arg;
	  goto next_insn;
	case RVM_FORK:
	  THREAD.ip++;
	  if (!insn_seen[arg]) {
	    insn_seen[THREAD.ip] = 2;
	    HRVMTrace* tr = THREAD.trace;
	    ipq_top++;
	    THREAD.ip = arg;
	    THREAD.trace = tr;
	  }
	  goto next_insn;
	case RVM_PUSH:
	  PUSH_SVM(SVM_PUSH, 0);
	  THREAD.ip++;
	  goto next_insn;
	case RVM_ACTION:
	  PUSH_SVM(SVM_ACTION, arg);
	  THREAD.ip++;
	  goto next_insn;
	case RVM_CAPTURE:
	  PUSH_SVM(SVM_CAPTURE, 0);
	  THREAD.ip++;
	  goto next_insn;
	case RVM_EOF:
	  THREAD.ip++;
	  if (off != len)
	    ipq_top--; // Terminate thread
	  goto next_insn;
	case RVM_STEP:
	  // save thread
	  live_threads++;
	  heads_n[THREAD.ip++] = THREAD.trace;
	  ipq_top--;
	  goto next_insn;
	}
      next_insn:
	;
	
      }
    }
  }
  // No accept was reached.
 match_fail:
  h_delete_arena(arena);
  return NULL;
  
 run_trace:
  // Invert the direction of the trace linked list.

  
  ret_trace = invert_trace(ret_trace);
  HParseResult *ret = run_trace(mm__, prog, ret_trace, input, len);
  // ret is in its own arena
  h_delete_arena(arena);
  return ret;
}
#undef PUSH_SVM
#undef THREAD




void svm_stack_ensure_cap(HAllocator *mm__, HSVMContext *ctx, size_t addl) {
  if (ctx->stack_count + addl >= ctx->stack_capacity) {
    ctx->stack = mm__->realloc(mm__, ctx->stack, sizeof(*ctx->stack) * (ctx->stack_capacity *= 2));
    // TODO: check for realloc failure
  }
}

HParseResult *run_trace(HAllocator *mm__, HRVMProg *orig_prog, HRVMTrace *trace, const uint8_t *input, int len) {
  // orig_prog is only used for the action table
  HSVMContext ctx;
  HArena *arena = h_new_arena(mm__, 0);
  ctx.stack_count = 0;
  ctx.stack_capacity = 16;
  ctx.stack = h_new(HParsedToken*, ctx.stack_capacity);

  HParsedToken *tmp_res;
  HRVMTrace *cur;
  for (cur = trace; cur; cur = cur->next) {
    switch (cur->opcode) {
    case SVM_PUSH:
      svm_stack_ensure_cap(mm__, &ctx, 1);
      tmp_res = a_new(HParsedToken, 1);
      tmp_res->token_type = TT_MARK;
      tmp_res->index = cur->input_pos;
      tmp_res->bit_offset = 0;
      ctx.stack[ctx.stack_count++] = tmp_res;
      break;
    case SVM_NOP:
      break;
    case SVM_ACTION:
      // Action should modify stack appropriately
      if (!orig_prog->actions[cur->arg].action(arena, &ctx, orig_prog->actions[cur->arg].env)) {
	// action failed... abort somehow
	// TODO: Actually abort
      }
      break;
    case SVM_CAPTURE: 
      // Top of stack must be a mark
      // This replaces said mark in-place with a TT_BYTES.
      assert(ctx.stack[ctx.stack_count]->token_type == TT_MARK);
      
      tmp_res = ctx.stack[ctx.stack_count];
      tmp_res->token_type = TT_BYTES;
      // TODO: Will need to copy if bit_offset is nonzero
      assert(tmp_res->bit_offset == 0);
	
      tmp_res->bytes.token = input + tmp_res->index;
      tmp_res->bytes.len = cur->input_pos - tmp_res->index + 1; // inclusive
      break;
    case SVM_ACCEPT:
      assert(ctx.stack_count == 1);
      HParseResult *res = a_new(HParseResult, 1);
      res->ast = ctx.stack[0];
      res->bit_length = cur->input_pos * 8;
      res->arena = arena;
      return res;
    }
  }

  h_delete_arena(arena);
  return NULL;
}

uint16_t h_rvm_create_action(HRVMProg *prog, HSVMActionFunc action_func, void* env) {
  for (uint16_t i = 0; i < prog->action_count; i++) {
    if (prog->actions[i].action == action_func && prog->actions[i].env == env)
      return i;
  }
  // Ensure that there's room in the action array...
  if (!(prog->action_count & (prog->action_count + 1))) {
    // needs to be scaled up.
    size_t array_size = (prog->action_count + 1) * 2; // action_count+1 is a
					       // power of two
    prog->actions = prog->allocator->realloc(prog->allocator, prog->actions, array_size * sizeof(*prog->actions));
    // TODO: Handle the allocation failed case nicely.
  }

  HSVMAction *action = &prog->actions[prog->action_count];
  action->action = action_func;
  action->env = env;
  return prog->action_count++;
}

uint16_t h_rvm_insert_insn(HRVMProg *prog, HRVMOp op, uint16_t arg) {
  // Ensure that there's room in the insn array...
  if (!(prog->length & (prog->length + 1))) {
    // needs to be scaled up.
    size_t array_size = (prog->length + 1) * 2; // action_count+1 is a
						// power of two
    prog->insns = prog->allocator->realloc(prog->allocator, prog->insns, array_size * sizeof(*prog->insns));
    // TODO: Handle the allocation failed case nicely.
  }

  prog->insns[prog->length].op = op;
  prog->insns[prog->length].arg = arg;
  return prog->length++;
}

uint16_t h_rvm_get_ip(HRVMProg *prog) {
  return prog->length;
}

void h_rvm_patch_arg(HRVMProg *prog, uint16_t ip, uint16_t new_val) {
  assert(prog->length > ip);
  prog->insns[ip].arg = new_val;
}

size_t h_svm_count_to_mark(HSVMContext *ctx) {
  size_t ctm;
  for (ctm = 0; ctm < ctx->stack_count-1; ctm++) {
    if (ctx->stack[ctx->stack_count - 1 - ctm]->token_type == TT_MARK)
      return ctm;
  }
  return ctx->stack_count;
}

// TODO: Implement the primitive actions
bool h_svm_action_make_sequence(HArena *arena, HSVMContext *ctx, void* env) {
  size_t n_items = h_svm_count_to_mark(ctx);
  assert (n_items < ctx->stack_count);
  HParsedToken *res = ctx->stack[ctx->stack_count - 1 - n_items];
  assert (res->token_type == TT_MARK);
  res->token_type = TT_SEQUENCE;
  
  HCountedArray *ret_carray = h_carray_new_sized(arena, n_items);
  res->seq = ret_carray;
  // res index and bit offset are the same as the mark.
  for (size_t i = 0; i < n_items; i++) {
    ret_carray->elements[i] = ctx->stack[ctx->stack_count - n_items + i];
  }
  ctx->stack_count -= n_items;
  return true;
}

bool h_svm_action_clear_to_mark(HArena *arena, HSVMContext *ctx, void* env) {
  while (ctx->stack_count > 0) {
    if (ctx->stack[--ctx->stack_count]->token_type == TT_MARK)
      return true;
  }
  return false; // no mark found.
}

// Glue regex backend to rest of system

bool h_compile_regex(HRVMProg *prog, const HParser *parser) {
  return parser->vtable->compile_to_rvm(prog, parser->env);
}

static void h_regex_free(HParser *parser) {
  HRVMProg *prog = (HRVMProg*)parser->backend_data;
  HAllocator *mm__ = prog->allocator;
  h_free(prog->insns);
  h_free(prog->actions);
  h_free(prog);
  parser->backend_data = NULL;
  parser->backend = PB_PACKRAT;
}

static int h_regex_compile(HAllocator *mm__, HParser* parser, const void* params) {
  if (!parser->vtable->isValidRegular(parser->env))
    return 1;
  HRVMProg *prog = h_new(HRVMProg, 1);
  prog->allocator = mm__;
  if (!h_compile_regex(prog, parser)) {
    h_free(prog->insns);
    h_free(prog->actions);
    h_free(prog);
    return 2;
  }
  parser->backend_data = prog;
  return 0;
}

static HParseResult *h_regex_parse(HAllocator* mm__, const HParser* parser, HInputStream *input_stream) {
  return h_rvm_run__m(mm__, (HRVMProg*)parser->backend_data, input_stream->input, input_stream->length);
}

HParserBackendVTable h__regex_backend_vtable = {
  .compile = h_regex_compile,
  .parse = h_regex_parse,
  .free = h_regex_free
};
