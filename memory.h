/**
 * @file memory.h
 * @brief Memory management function prototypes.
 */

#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"

void free_safe(void *pointer, size_t size, SystemState *sys);
void clean_all(SystemState *sys);
void *safemalloc(size_t size, SystemState *sys);
void *safe_realloc(void *pointer, size_t old_size, size_t new_size, SystemState *sys);

#endif