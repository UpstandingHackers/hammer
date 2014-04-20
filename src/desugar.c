#include "hammer.h"
#include "internal.h"
#include "backends/contextfree.h"

HCFChoice *h_desugar(HAllocator *mm__, HCFStack *stk__, const HParser *parser) {
  HCFStack *nstk__ = stk__;
  if(parser->desugared == NULL) {
    if (nstk__ == NULL) {
      nstk__ = h_cfstack_new(mm__);
    }
    if (nstk__->prealloc == NULL) {
      nstk__->prealloc = h_new(HCFChoice, 1);
    }
    // we're going to do something naughty and cast away the const to memoize
    assert(parser->vtable->desugar != NULL);
    ((HParser *)parser)->desugared = nstk__->prealloc;
    parser->vtable->desugar(mm__, nstk__, parser->env);
    if (stk__ == NULL) {
      h_cfstack_free(mm__, nstk__);
    }
  } else if (stk__ != NULL) {
    HCFS_APPEND(parser->desugared);
  }

  return parser->desugared;
}
