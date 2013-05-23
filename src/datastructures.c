#include "internal.h"
#include "hammer.h"
#include "allocator.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
// {{{ counted arrays


HCountedArray *h_carray_new_sized(HArena * arena, size_t size) {
  HCountedArray *ret = h_arena_malloc(arena, sizeof(HCountedArray));
  assert(size > 0);
  ret->used = 0;
  ret->capacity = size;
  ret->arena = arena;
  ret->elements = h_arena_malloc(arena, sizeof(void*) * size);
  return ret;
}

HCountedArray *h_carray_new(HArena * arena) {
  return h_carray_new_sized(arena, 4);
}

void h_carray_append(HCountedArray *array, void* item) {
  if (array->used >= array->capacity) {
    HParsedToken **elements = h_arena_malloc(array->arena, (array->capacity *= 2) * sizeof(HCountedArray*));
    for (size_t i = 0; i < array->used; i++)
      elements[i] = array->elements[i];
    for (size_t i = array->used; i < array->capacity; i++)
      elements[i] = 0;
    array->elements = elements;
  }
  array->elements[array->used++] = item;
}

// HSlist
HSlist* h_slist_new(HArena *arena) {
  HSlist *ret = h_arena_malloc(arena, sizeof(HSlist));
  ret->head = NULL;
  ret->arena = arena;
  return ret;
}

HSlist* h_slist_copy(HSlist *slist) {
  HSlist *ret = h_slist_new(slist->arena);
  HSlistNode *head = slist->head;
  HSlistNode *tail;
  if (head != NULL) {
    h_slist_push(ret, head->elem);
    tail = ret->head;
    head = head->next;
  }
  while (head != NULL) {
    // append head item to tail in a new node
    HSlistNode *node = h_arena_malloc(slist->arena, sizeof(HSlistNode));
    node->elem = head->elem;
    node->next = NULL;
    tail = tail->next = node;
    head = head->next;
  }
  return ret;
}

void* h_slist_pop(HSlist *slist) {
  HSlistNode *head = slist->head;
  if (!head)
    return NULL;
  void* ret = head->elem;
  slist->head = head->next;
  h_arena_free(slist->arena, head);
  return ret;
}

void h_slist_push(HSlist *slist, void* item) {
  HSlistNode *hnode = h_arena_malloc(slist->arena, sizeof(HSlistNode));
  hnode->elem = item;
  hnode->next = slist->head;
  // write memory barrier here.
  slist->head = hnode;
}

bool h_slist_find(HSlist *slist, const void* item) {
  assert (item != NULL);
  HSlistNode *head = slist->head;
  while (head != NULL) {
    if (head->elem == item)
      return true;
    head = head->next;
  }
  return false;
}

HSlist* h_slist_remove_all(HSlist *slist, const void* item) {
  assert (item != NULL);
  HSlistNode *node = slist->head;
  HSlistNode *prev = NULL;
  while (node != NULL) {
    if (node->elem == item) {
      HSlistNode *next = node->next;
      if (prev)
	prev->next = next;
      else
	slist->head = next;
      // FIXME free the removed node! this leaks.
      node = next;
    }
    else {
      prev = node;
      node = prev->next;
    }
  }
  return slist;
}

void h_slist_free(HSlist *slist) {
  while (slist->head != NULL)
    h_slist_pop(slist);
  h_arena_free(slist->arena, slist);
}

HHashTable* h_hashtable_new(HArena *arena, HEqualFunc equalFunc, HHashFunc hashFunc) {
  HHashTable *ht = h_arena_malloc(arena, sizeof(HHashTable));
  ht->hashFunc = hashFunc;
  ht->equalFunc = equalFunc;
  ht->capacity = 64; // to start; should be tuned later...
  ht->used = 0;
  ht->arena = arena;
  ht->contents = h_arena_malloc(arena, sizeof(HHashTableEntry) * ht->capacity);
  for (size_t i = 0; i < ht->capacity; i++) {
    ht->contents[i].key = NULL;
    ht->contents[i].value = NULL;
    ht->contents[i].next = NULL;
    ht->contents[i].hashval = 0;
  }
  //memset(ht->contents, 0, sizeof(HHashTableEntry) * ht->capacity);
  return ht;
}

void* h_hashtable_get(const HHashTable* ht, const void* key) {
  HHashValue hashval = ht->hashFunc(key);
#ifdef CONSISTENCY_CHECK
  assert((ht->capacity & (ht->capacity - 1)) == 0); // capacity is a power of 2
#endif

  HHashTableEntry *hte = NULL;
  for (hte = &ht->contents[hashval & (ht->capacity - 1)];
       hte != NULL;
       hte = hte->next) {
    if (hte->hashval != hashval)
      continue;
    if (ht->equalFunc(key, hte->key))
      return hte->value;
  }
  return NULL;
}

void h_hashtable_put(HHashTable* ht, const void* key, void* value) {
  // # Start with a rebalancing
  //h_hashtable_ensure_capacity(ht, ht->used + 1);

  HHashValue hashval = ht->hashFunc(key);
#ifdef CONSISTENCY_CHECK
  assert((ht->capacity & (ht->capacity - 1)) == 0); // capacity is a power of 2
#endif

  HHashTableEntry *hte = &ht->contents[hashval & (ht->capacity - 1)];
  if (hte->key != NULL) {
    for(;;) {
      // check each link, stay on last if not found
      if (hte->hashval == hashval && ht->equalFunc(key, hte->key))
	goto insert_here;
      if (hte->next == NULL)
        break;
      hte = hte->next;
    }
    // Add a new link...
    assert (hte->next == NULL);
    hte->next = h_arena_malloc(ht->arena, sizeof(HHashTableEntry));
    hte = hte->next;
    hte->next = NULL;
    ht->used++;
  } else
    ht->used++;

 insert_here:
  hte->key = key;
  hte->value = value;
  hte->hashval = hashval;
}

void h_hashtable_update(HHashTable *dst, const HHashTable *src) {
  size_t i;
  HHashTableEntry *hte;
  for(i=0; i < src->capacity; i++) {
    for(hte = &src->contents[i]; hte; hte = hte->next) {
      if(hte->key == NULL)
        continue;
      h_hashtable_put(dst, hte->key, hte->value);
    }
  }
}

void h_hashtable_merge(void *(*combine)(void *v1, void *v2),
	HHashTable *dst, const HHashTable *src) {
  size_t i;
  HHashTableEntry *hte;
  for(i=0; i < src->capacity; i++) {
    for(hte = &src->contents[i]; hte; hte = hte->next) {
      if(hte->key == NULL)
        continue;
      void *oldvalue = h_hashtable_get(dst, hte->key);
      void *newvalue;
      if(oldvalue)
        newvalue = combine(oldvalue, hte->value);
      else
        newvalue = hte->value;
      h_hashtable_put(dst, hte->key, newvalue);
    }
  }
}

int   h_hashtable_present(const HHashTable* ht, const void* key) {
  HHashValue hashval = ht->hashFunc(key);
#ifdef CONSISTENCY_CHECK
  assert((ht->capacity & (ht->capacity - 1)) == 0); // capacity is a power of 2
#endif

  for (HHashTableEntry *hte = &ht->contents[hashval & (ht->capacity - 1)];
       hte != NULL;
       hte = hte->next) {
    if (hte->hashval != hashval)
      continue;
    if (ht->equalFunc(key, hte->key))
      return true;
  }
  return false;
}
void  h_hashtable_del(HHashTable* ht, const void* key) {
  HHashValue hashval = ht->hashFunc(key);
#ifdef CONSISTENCY_CHECK
  assert((ht->capacity & (ht->capacity - 1)) == 0); // capacity is a power of 2
#endif

  for (HHashTableEntry *hte = &ht->contents[hashval & (ht->capacity - 1)];
       hte != NULL;
       hte = hte->next) {
    if (hte->hashval != hashval)
      continue;
    if (ht->equalFunc(key, hte->key)) {
      // FIXME: Leaks keys and values.
      HHashTableEntry* hten = hte->next;
      if (hten != NULL) {
	*hte = *hten;
	h_arena_free(ht->arena, hten);
      } else {
	hte->key = hte->value = NULL;
	hte->hashval = 0;
      }
      return;
    }
  }
}
void  h_hashtable_free(HHashTable* ht) {
  for (size_t i = 0; i < ht->capacity; i++) {
    HHashTableEntry *hten, *hte = &ht->contents[i];
    // FIXME: Free key and value
    hte = hte->next;
    while (hte != NULL) {
      // FIXME: leaks keys and values.
      hten = hte->next;
      h_arena_free(ht->arena, hte);
      hte = hten;
    }
  }
  h_arena_free(ht->arena, ht->contents);
}

bool h_eq_ptr(const void *p, const void *q) {
  return (p==q);
}

HHashValue h_hash_ptr(const void *p) {
  // XXX just djbhash it
  return (uintptr_t)p >> 4;
}
