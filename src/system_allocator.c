#include <stdlib.h>
#include "internal.h"

static void* system_alloc(HAllocator *allocator, size_t size) {
  return malloc(size);
}

static void* system_realloc(HAllocator *allocator, void* ptr, size_t size) {
  return realloc(ptr, size);
}

static void system_free(HAllocator *allocator, void* ptr) {
  free(ptr);
}

HAllocator system_allocator = {
  .alloc = system_alloc,
  .realloc = system_realloc,
  .free = system_free,
};
