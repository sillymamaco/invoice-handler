/**
 * @file memory.h
 * @brief Tracked memory management: allocation, resizing, and cleanup.
 *
 * @details Every allocation is reflected in ::SystemState::memory_used so
 * that the program can enforce the ::MAX_MEMORY_ALLOCATED hard limit.
 * All functions terminate the program gracefully — printing @c "No memory."
 * and releasing all heap memory — when the limit is exceeded or when
 * @c malloc / @c realloc returns @c NULL.
 */

#ifndef MEMORY_H
#define MEMORY_H

#include "common.h"

/**
 * @brief Decrement the memory counter then free a previously tracked block.
 *
 * @details Guards against counter underflow: if @p size exceeds
 * @c sys->memory_used the counter is clamped to zero rather than wrapping.
 * Passing @c NULL for @p pointer is safe and has no effect.
 *
 * @param pointer Pointer to the block to free; ignored when @c NULL.
 * @param size    Byte count originally passed to safemalloc() or
 * safe_realloc().
 * @param sys     System state whose @c memory_used counter is decremented.
 */
void free_safe(void *pointer, size_t size, SystemState *sys);

/**
 * @brief Release every heap allocation owned by @p sys.
 *
 * @details Frees the basket array, all ::ClientRecord entries (including their
 * invoice arrays and name strings), and the product catalog (including all
 * description strings). Freed pointers are set to @c NULL so the function is
 * safe to call more than once.
 *
 * @param sys System state to clean up.
 */
void clean_all(SystemState *sys);

/**
 * @brief Allocate @p size bytes and track the allocation in @p sys.
 *
 * @details Calls clean_all() then @c exit(0) when the ::MAX_MEMORY_ALLOCATED
 * budget would be exceeded or when @c malloc returns @c NULL.
 *
 * @param size Number of bytes to allocate; returns @c NULL when zero.
 * @param sys  System state whose @c memory_used counter is incremented.
 * @return Pointer to the newly allocated block (never @c NULL for size > 0).
 */
void *safemalloc(size_t size, SystemState *sys);

/**
 * @brief Resize a tracked allocation from @p old_size to @p new_size bytes.
 *
 * @details When growing, the budget check is performed before calling
 * @c realloc. The memory counter is adjusted after a confirmed success.
 * Calls clean_all() then @c exit(0) when the budget is exceeded or when
 * @c realloc fails.
 *
 * @param pointer  Pointer to the block to resize.
 * @param old_size Current size of the block in bytes.
 * @param new_size Desired size of the block in bytes.
 * @param sys      System state whose @c memory_used counter is adjusted.
 * @return Pointer to the resized block (may differ from @p pointer).
 */
void *safe_realloc(void *pointer, size_t old_size, size_t new_size,
                   SystemState *sys);

#endif /* MEMORY_H */