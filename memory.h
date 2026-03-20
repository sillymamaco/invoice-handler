/**
 * @file memory.h
 * @author IST1117890 (Irina Cojocari)
 * @brief Tracked memory management: allocation, resizing, and cleanup.
 *
 * @details Every allocation is reflected in SystemState memory_used so
 * that the program can enforce the MAX_MEMORY_ALLOCATED hard limit.
 * All functions terminate the program gracefully — printing "No memory."
 * and releasing all heap memory — when the limit is exceeded or when
 * malloc / realloc returns NULL.
 */

#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"

/**
 * @brief Decrement the memory counter then free a previously tracked block.
 *
 * @details Guards against counter underflow: if size exceeds
 * sys->memory_used the counter is clamped to zero rather than wrapping.
 * Passing NULL for pointer is safe and has no effect.
 *
 * @param Pointer to the block to free; ignored when NULL.
 * @param Byte count originally passed to safemalloc() or safe_realloc().
 * @param System state whose memory_used counter is decremented.
 */
void free_safe(void *pointer, size_t size, SystemState *sys);

/**
 * @brief Release every heap allocation owned by @p sys.
 *
 * @details Frees the basket array, all ClientRecord entries (including their
 * invoice arrays and name strings), and the product catalog (including all
 * description strings). Freed pointers are set to NULL so the function is
 * safe to call more than once.
 *
 * @param System state to clean up.
 */
void clean_all(SystemState *sys);

/**
 * @brief Allocate size bytes and track the allocation in sys.
 *
 * @details Calls clean_all() then exit(0) when the MAX_MEMORY_ALLOCATED
 * budget would be exceeded or when malloc returns NULL.
 *
 * @param Number of bytes to allocate; returns NULL when zero.
 * @param System state whose memory_used counter is incremented.
 * @return Pointer to the newly allocated block (never NULL for size > 0).
 */
void *safemalloc(size_t size, SystemState *sys);

/**
 * @brief Resize a tracked allocation from old_size to new_size bytes.
 *
 * @details When growing, the budget check is performed before calling
 * realloc. The memory counter is adjusted after a confirmed success.
 * Calls clean_all() then exit(0) when the budget is exceeded or when
 * realloc fails.
 *
 * @param Pointer to the block to resize.
 * @param Current size of the block in bytes.
 * @param Desired size of the block in bytes.
 * @param System state whose memory_used counter is adjusted.
 * @return Pointer to the resized block (may differ from pointer).
 */
void *safe_realloc(void *pointer, size_t old_size, size_t new_size,
                   SystemState *sys);

#endif /* MEMORY_H */
