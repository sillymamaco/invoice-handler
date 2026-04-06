/*
 * Command 'p' prototypes and core memory/product utilities.
 * @file p.h
 * @author IST1117890 (Irina Cojocari)
 */

#ifndef P_H
#define P_H

#include "main.h"

/*
 * Decrement the memory counter then free a previously tracked block.
 * @param pointer Pointer to the block to free.
 * @param size Byte count originally allocated.
 * @param sys System state.
 */
void free_safe(void *pointer, size_t size, SystemState *sys);

/*
 * Release every dynamic allocation owned by sys.
 * @param sys System state to clean up.
 */
void clean_all(SystemState *sys);

/*
 * Allocate size bytes and track the allocation in sys.
 * @param size Number of bytes to allocate.
 * @param sys System state.
 * @return Pointer to the newly allocated block.
 */
void *safemalloc(size_t size, SystemState *sys);

/*
 * Resize a tracked allocation.
 * @param pointer Pointer to the block to resize.
 * @param old_size Current size of the block.
 * @param new_size Desired size of the block.
 * @param sys System state.
 * @return Pointer to the resized block.
 */
void *safe_realloc(void *pointer, size_t old_size, size_t new_size,
                   SystemState *sys);

/*
 * bsearch comparator: EAN string key vs Product element.
 * @param key Pointer to an EAN string.
 * @param elem Pointer to a product.
 * @return Comparison result.
 */
int cmp_product_search(const void *key, const void *elem);

/*
 * Binary-search the catalog for a product by EAN.
 * @param sys System state.
 * @param ean EAN string to find.
 * @return Index or -1 if not found.
 */
int find_product_idx(SystemState *sys, const char *ean);

/*
 * Validate an EAN code.
 * @param ean EAN string.
 * @return Non-zero if valid.
 */
int validate_ean(const char *ean);

/*
 * Check that a description starts with a valid character.
 * @param s Description string.
 * @return Non-zero if valid.
 */
int is_valid_desc_start(const char *s);

/*
 * Insert or update a product in the catalog.
 * @param sys System state.
 * @param table IVA rate table.
 */
void cmd_p(SystemState *sys, Iva table[]);

#endif /* P_H */
