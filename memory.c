/**
 * @file memory.c
 * @brief Memory management functions handling allocation and tracking.
 */

#include "memory.h"

void free_safe(void *pointer, size_t size, SystemState *sys) {
  if (pointer) {
    if (sys->memory_used >= size)
      sys->memory_used -= size;
    else
      sys->memory_used = 0;
    free(pointer);
  }
}

void clean_all(SystemState *sys) {
  if (sys->basket)
    free_safe(sys->basket, sys->basket_capacity * sizeof(BasketItem), sys);
  if (sys->history) {
    for (int i = 0; i < sys->history_count; i++)
      if (sys->history[i].client_name)
        free_safe(sys->history[i].client_name, strlen(sys->history[i].client_name) + 1, sys);
    free_safe(sys->history, sys->history_capacity * sizeof(Invoice), sys);
  }
  if (sys->catalog) {
    for (int i = 0; i < sys->catalog_count; i++)
      if (sys->catalog[i].desc)
        free_safe(sys->catalog[i].desc, strlen(sys->catalog[i].desc) + 1, sys);
    free_safe(sys->catalog, sys->catalog_capacity * sizeof(Product), sys);
  }
}

static void no_memory(SystemState *sys) {
  printf("No memory.\n");
  clean_all(sys);
  exit(0);
}

void *safemalloc(size_t size, SystemState *sys) {
  if ((sys->memory_used + size) > MAX_MEMORY_ALLOCATED)
    no_memory(sys);
  void *p = malloc(size);
  if (!p)
    no_memory(sys);
  sys->memory_used += size;
  return p;
}

void *safe_realloc(void *pointer, size_t old_size, size_t new_size, SystemState *sys) {
  size_t diff = (new_size > old_size) ? (new_size - old_size) : 0;
  if (new_size > old_size && (sys->memory_used + diff) > MAX_MEMORY_ALLOCATED)
    no_memory(sys);
  void *n = realloc(pointer, new_size);
  if (!n && new_size > 0)
    no_memory(sys); 
  if (new_size > old_size)
    sys->memory_used += diff;
  else
    sys->memory_used -= (old_size - new_size);
  return n;
}