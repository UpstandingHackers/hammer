/* Arena allocator for Hammer.
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

#ifndef HAMMER_ALLOCATOR__H__
#define HAMMER_ALLOCATOR__H__
#include <sys/types.h>
#include "export.h"

#ifdef __cplusplus
extern "C" {
#endif

// TODO(thequux): Turn this into an "HAllocatorVtable", and add a wrapper that also takes an environment pointer.
typedef struct HAllocator_ {
  void* (*alloc)(struct HAllocator_* allocator, size_t size);
  void* (*realloc)(struct HAllocator_* allocator, void* ptr, size_t size);
  void (*free)(struct HAllocator_* allocator, void* ptr);
} HAllocator;

typedef struct HArena_ HArena ; // hidden implementation

H_EXPORT HArena *h_new_arena(HAllocator* allocator, size_t block_size); // pass 0 for default...
#ifndef SWIG
H_EXPORT void* h_arena_malloc(HArena *arena, size_t count) __attribute__(( malloc, alloc_size(2) ));
#else
H_EXPORT void* h_arena_malloc(HArena *arena, size_t count);
#endif
H_EXPORT void h_arena_free(HArena *arena, void* ptr); // For future expansion, with alternate memory managers.
H_EXPORT void h_delete_arena(HArena *arena);

typedef struct {
  size_t used;
  size_t wasted;
} HArenaStats;

H_EXPORT void h_allocator_stats(HArena *arena, HArenaStats *stats);

#ifdef __cplusplus
}
#endif

#endif // #ifndef HAMMER_ALLOCATOR__H__
