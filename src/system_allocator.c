#include <string.h>
#include <stdlib.h>
#include "internal.h"

static void* system_alloc(HAllocator *allocator, size_t size) {
  
  void* ptr = calloc(size + sizeof(size_t), 1);
  *(size_t*)ptr = size;
  return ptr + sizeof(size_t);
}

static void* system_realloc(HAllocator *allocator, void* ptr, size_t size) {
  if (ptr == NULL)
    return system_alloc(allocator, size);
  ptr = realloc(ptr - sizeof(size_t), size + sizeof(size_t));
  size_t old_size = *(size_t*)ptr;
  *(size_t*)ptr = size;
  if (size > old_size)
    memset(ptr+sizeof(size_t)+old_size, 0, size - old_size);
  return ptr + sizeof(size_t);
}

static void system_free(HAllocator *allocator, void* ptr) {
  free(ptr - sizeof(size_t));
}

HAllocator system_allocator = {
  .alloc = system_alloc,
  .realloc = system_realloc,
  .free = system_free,
};
