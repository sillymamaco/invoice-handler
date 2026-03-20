/**
 * @file memory.c
 * @author IST1117890 (Irina Cojocari)
 * @brief Tracked memory management: allocation, resizing, and cleanup.
 */

#include "memory.h"

void free_safe(void *pointer, size_t size, SystemState *sys) {
  if (pointer) {
    sys->memory_used = (sys->memory_used >= size) ? sys->memory_used - size : 0;
    free(pointer);
  }
}

void clean_all(SystemState *sys) {
  if (sys->basket) {
    free_safe(sys->basket, sys->basket_capacity * sizeof(BasketItem), sys);
    sys->basket = NULL;
  }

  if (sys->clients) {
    for (int i = 0; i < sys->client_count; i++) {
      ClientRecord *cr = &sys->clients[i];
      if (cr->invoices)
        free_safe(cr->invoices, cr->invoice_cap * sizeof(Invoice), sys);
      if (cr->name)
        free_safe(cr->name, strlen(cr->name) + 1, sys);
    }
    free_safe(sys->clients, sys->client_capacity * sizeof(ClientRecord), sys);
    sys->clients = NULL;
  }

  if (sys->catalog) {
    for (int i = 0; i < sys->catalog_count; i++)
      if (sys->catalog[i].desc)
        free_safe(sys->catalog[i].desc, strlen(sys->catalog[i].desc) + 1, sys);
    free_safe(sys->catalog, sys->catalog_capacity * sizeof(Product), sys);
    sys->catalog = NULL;
  }
}

/**
 * @brief Print @c "No memory.", release all heap memory, and terminate.
 *
 * @param sys System state to clean before calling @c exit(0).
 */
static void no_memory(SystemState *sys) {
  printf("No memory.\n");
  clean_all(sys);
  exit(0);
}

void *safemalloc(size_t size, SystemState *sys) {
  if (size == 0)
    return NULL;
  if (sys->memory_used + size > (size_t)MAX_MEMORY_ALLOCATED)
    no_memory(sys);
  void *p = malloc(size);
  if (!p)
    no_memory(sys);
  sys->memory_used += size;
  return p;
}

void *safe_realloc(void *pointer, size_t old_size, size_t new_size,
                   SystemState *sys) {
  if (new_size > old_size) {
    size_t extra = new_size - old_size;
    if (sys->memory_used + extra > (size_t)MAX_MEMORY_ALLOCATED)
      no_memory(sys);
  }
  void *n = realloc(pointer, new_size);
  if (!n && new_size > 0)
    no_memory(sys);

  if (new_size >= old_size)
    sys->memory_used += (new_size - old_size);
  else
    sys->memory_used -= (old_size - new_size);
  return n;
}
