/**
 * @file memory.h
 * @author ist1117890
 * @brief Memory management and cleanup functions.
 */
#ifndef MEMORY_H
#define MEMORY_H

#include "types.h"
#include <stddef.h>

/**
 * @brief Safely frees a pointer and updates memory usage.
 * @param pointer Pointer to free.
 * @param size Size of the memory block being freed.
 * @param sys Pointer to the system state.
 */
void free_safe(void *pointer, size_t size, SystemState *sys);

/**
 * @brief Cleans up all allocated memory in the system.
 * @param sys Pointer to the system state.
 */
void clean_all(SystemState *sys);

/**
 * @brief Exits the program when memory limit is exceeded.
 * @param sys Pointer to the system state.
 */
void no_memory(SystemState *sys);

/**
 * @brief Allocates memory safely, tracking usage limits.
 * @param size Amount of bytes to allocate.
 * @param sys Pointer to the system state.
 * @return Pointer to the allocated memory.
 */
void *safemalloc(size_t size, SystemState *sys);

/**
 * @brief Reallocates memory safely, tracking usage limits.
 * @param pointer Pointer to the old memory block.
 * @param old_size Old size of the memory block.
 * @param new_size New size to allocate.
 * @param sys Pointer to the system state.
 * @return Pointer to the new memory block.
 */
void *safe_realloc(void *pointer, size_t old_size, size_t new_size,
                   SystemState *sys);

/**
 * @brief Reads a token from standard input safely.
 * @param sys Pointer to the system state.
 * @return Pointer to the dynamically allocated string token.
 */
char *read_token_safe(SystemState *sys);

#endif