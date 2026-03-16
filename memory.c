#include "memory.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void free_safe(void *pointer, size_t size, SystemState *sys) {
  if (pointer) {
    sys->memory_used -= (int)size;
    free(pointer);
  }
}

void clean_all(SystemState *sys) {
  if (sys->basket)
    free_safe(sys->basket, sys->basket_capacity * sizeof(BasketItem), sys);
  if (sys->history) {
    for (int i = 0; i < sys->history_count; i++)
      if (sys->history[i].client_name)
        free_safe(sys->history[i].client_name,
                  strlen(sys->history[i].client_name) + 1, sys);
    free_safe(sys->history, sys->history_capacity * sizeof(Invoice), sys);
  }
  if (sys->catalog) {
    for (int i = 0; i < sys->catalog_count; i++)
      if (sys->catalog[i].desc)
        free_safe(sys->catalog[i].desc, strlen(sys->catalog[i].desc) + 1, sys);
    free_safe(sys->catalog, sys->catalog_capacity * sizeof(Product), sys);
  }
}

void no_memory(SystemState *sys) {
  printf("No memory.\n");
  clean_all(sys);
  exit(0);
}

void *safemalloc(size_t size, SystemState *sys) {
  if ((sys->memory_used + (int)size) > MAX_MEMORY_ALLOCATED)
    no_memory(sys);
  void *p = malloc(size);
  if (p)
    sys->memory_used += (int)size;
  return p;
}

void *safe_realloc(void *pointer, size_t old_size, size_t new_size,
                   SystemState *sys) {
  int diff = (int)new_size - (int)old_size;
  if ((sys->memory_used + diff) > MAX_MEMORY_ALLOCATED)
    no_memory(sys);
  void *n = realloc(pointer, new_size);
  if (n)
    sys->memory_used += diff;
  return n;
}

char *read_token_safe(SystemState *sys) {
  char buf[MAX_INSTRC_LENGTH] = {0};
  int c, i = 0;
  while ((c = getchar()) != '\n' && c != EOF && isspace(c))
    ;
  if (c != '\n' && c != EOF) {
    buf[i++] = (char)c;
    while ((c = getchar()) != '\n' && c != EOF)
      if (i < MAX_INSTRC_LENGTH && c != '\r')
        buf[i++] = (char)c;
  }
  while (i > 0 && isspace((unsigned char)buf[i - 1]))
    i--;
  buf[i] = '\0';
  if (i == 0)
    return NULL;
  char *res = safemalloc(i + 1, sys);
  if (res)
    strcpy(res, buf);
  return res;
}