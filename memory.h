/**
 * @file memory.h
 * @author IST1117890 (Irina Cojocari)
 * @brief Tracked memory management: allocation, resizing, and cleanup.
 */

#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"

/**
 * @brief Decrement the memory counter then free a previously tracked block.
 * @param pointer Pointer to the block to free.
 * @param size Byte count originally allocated.
 * @param sys System state.
 */
void free_safe(void *pointer, size_t size, SystemState *sys);

/**
 * @brief Release every heap allocation owned by sys.
 * @param sys System state to clean up.
 */
void clean_all(SystemState *sys);

/**
 * @brief Allocate size bytes and track the allocation in sys.
 * @param size Number of bytes to allocate.
 * @param sys System state.
 * @return Pointer to the newly allocated block.
 */
void *safemalloc(size_t size, SystemState *sys);

/**
 * @brief Resize a tracked allocation.
 * @param pointer Pointer to the block to resize.
 * @param old_size Current size of the block.
 * @param new_size Desired size of the block.
 * @param sys System state.
 * @return Pointer to the resized block.
 */
void *safe_realloc(void *pointer, size_t old_size, size_t new_size,
                   SystemState *sys);

#endif /* MEMORY_H */