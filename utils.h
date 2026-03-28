/**
 * Prototypes for utility functions.
 * @file utils.h
 * @author IST1117890 (Irina Cojocari)
 */

#ifndef UTILS_H
#define UTILS_H

#include "common.h"

/**
 * Fill buffer from stdin until newline or EOF.
 * @param buf Buffer to fill.
 * @param i Pointer to index.
 * @param truncated Pointer to truncation flag (for big inputs that exceed
 *                  limit such as client names).
 */
void fill_buffer_from_stdin(char *buf, int *i, int *truncated);

/**
 * Extract a string handling quotes.
 * @param ptr Pointer to input stream.
 * @param name_buf Output buffer.
 * @param name_buf_size Buffer size.
 * @return Non-zero on success.
 */
int extract_quoted_string(const char **ptr, char *name_buf,
                          size_t name_buf_size);

/**
 * Read a full line from stdin into a buffer.
 * @param buf Output buffer.
 * @param limit Buffer size limit.
 * @return Non-zero if line was truncated.
 */
int read_line_to_buffer(char *buf, size_t limit);

/**
 * Read the rest of the current stdin line as a single token.
 * @param sys System state.
 * @return Dynamically allocated string, or NULL for a blank line.
 */
char *read_token_safe(SystemState *sys);

/**
 * bsearch comparator: EAN string key vs Product element.
 * @param key Pointer to an EAN string.
 * @param elem Pointer to a product.
 * @return Comparison result.
 */
int cmp_product_search(const void *key, const void *elem);

/**
 * Binary-search the catalog for a product by EAN.
 * @param sys System state.
 * @param ean EAN string to find.
 * @return Index or -1 if not found.
 */
int find_product_idx(SystemState *sys, const char *ean);

/**
 * Check whether an IVA class letter is defined.
 * @param table IVA rate table.
 * @param letter Class letter.
 * @return Non-zero if present.
 */
int iva_is_present(const Iva table[], char letter);

/**
 * Retrieve the tax rate for an IVA class letter.
 * @param table IVA rate table.
 * @param letter Class letter.
 * @return Tax percentage.
 */
int get_iva_rate(const Iva table[], char letter);

/**
 * Define or update one slot in the IVA rate table.
 * @param table IVA rate table.
 * @param letter Class letter.
 * @param value Tax percentage.
 */
void iva_set(Iva table[], char letter, int value);

/**
 * Validate an EAN code.
 * @param ean EAN string.
 * @return Non-zero if valid.
 */
int validate_ean(const char *ean);

/**
 * Check that a description starts with a valid character.
 * @param s Description string.
 * @return Non-zero if valid.
 */
int is_valid_desc_start(const char *s);

/**
 * Check that a client name starts with a valid character.
 * @param s Name string.
 * @return Non-zero if valid.
 */
int is_valid_name_start(const char *s);

/**
 * Validate fields of a p command.
 * @param ean EAN string.
 * @param iva_ok IVA validity flag.
 * @param price Parsed price.
 * @param stock Parsed quantity.
 * @param desc Description string.
 * @return Non-zero if valid.
 */
int validate_p_input(const char *ean, int iva_ok, double price, int stock,
                     const char *desc);

/**
 * Test whether text matches a given pattern with wildcards.
 * @param pattern Wildcard pattern.
 * @param text Text to match.
 * @return Non-zero if match is found.
 */
int match(const char *pattern, const char *text);

/**
 * Print one product line.
 * @param p Pointer to the product.
 */
void print_product(const Product *p);

/**
 * Print one basket line.
 * @param sys System state.
 * @param table IVA rate table.
 * @param cat_idx Index of the product in the catalog.
 * @param qty Quantity to display.
 */
void print_basket_item(SystemState *sys, const Iva table[], int cat_idx,
                       int qty);

/**
 * Compare two client name strings.
 * @param a First name.
 * @param b Second name.
 * @return Comparison result.
 */
int cmp_names(const char *a, const char *b);

/**
 * Binary-search the client array for a record by name.
 * @param sys System state.
 * @param name Client name.
 * @return Index or -1 if not found.
 */
int find_client_idx(SystemState *sys, const char *name);

#endif /* UTILS_H */
