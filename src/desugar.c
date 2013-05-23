#include "hammer.h"
#include "internal.h"

HCFChoice *h_desugar(HAllocator *mm__, const HParser *parser) {
  if(parser->desugared == NULL) {
    // we're going to do something naughty and cast away the const to memoize
    ((HParser *)parser)->desugared = parser->vtable->desugar(mm__, parser->env);
  }

  return parser->desugared;
}
