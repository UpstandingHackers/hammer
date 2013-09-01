// Intended to be included from regex_debug.c
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>



// This is some spectacularly non-portable code... but whee!
#include <dlfcn.h>
char* getsym(void* addr) {
  char* retstr;
#if 0
  // This will be fixed later.
  Dl_info dli;
  if (dladdr(addr, &dli) != 0 && dli.dli_sname != NULL) {
    if (dli.dli_saddr == addr)
      return strdup(dli.dli_sname);
    else if (asprintf(&retstr, "%s+0x%lx", dli.dli_sname, addr - dli.dli_saddr) > 0)
      return retstr;
  } else
#endif
    if (asprintf(&retstr, "%p", addr) > 0)
      return retstr;
    else
      return NULL;
}

const char* rvm_op_names[RVM_OPCOUNT] = {
  "ACCEPT",
  "GOTO",
  "FORK",
  "PUSH",
  "ACTION",
  "CAPTURE",
  "EOF",
  "MATCH",
  "STEP"
};

const char* svm_op_names[SVM_OPCOUNT] = {
  "PUSH",
  "NOP",
  "ACTION",
  "CAPTURE",
  "ACCEPT"
};

void dump_rvm_prog(HRVMProg *prog) {
  char* symref;
  for (unsigned int i = 0; i < prog->length; i++) {
    HRVMInsn *insn = &prog->insns[i];
    printf("%4d %-10s", i, rvm_op_names[insn->op]);
    switch (insn->op) {
    case RVM_GOTO:
    case RVM_FORK:
      printf("%hd\n", insn->arg);
      break;
    case RVM_ACTION:
      symref = getsym(prog->actions[insn->arg].action);
      // TODO: somehow format the argument to action
      printf("%s\n", symref);
      free(symref);
      break;
    case RVM_MATCH: {
      uint8_t low, high;
      low = insn->arg & 0xff;
      high = (insn->arg >> 8) & 0xff;
      if (high < low)
	printf("NONE\n");
      else {
	if (low >= 0x20 && low <= 0x7e)
	  printf("%02hhx ('%c')", low, low);
	else
	  printf("%02hhx", low);
	
	if (high >= 0x20 && high <= 0x7e)
	  printf(" - %02hhx ('%c')\n", high, high);
	else
	  printf(" - %02hhx\n", high);
      }
      break;
    }
    default:
      printf("\n");
    }
  }
}

void dump_svm_prog(HRVMProg *prog, HRVMTrace *trace) {
  char* symref;
  for (; trace != NULL; trace = trace->next) {
    printf("@%04zd %-10s", trace->input_pos, svm_op_names[trace->opcode]);
    switch (trace->opcode) {
    case SVM_ACTION:
      symref = getsym(prog->actions[trace->arg].action);
      // TODO: somehow format the argument to action
      printf("%s\n", symref);
      free(symref);
      break;
    default:
      printf("\n");
    }
  }
}
