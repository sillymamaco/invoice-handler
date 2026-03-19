/**
 * @file utils.h
 * @brief Prototypes for utility functions used across the billing system.
 *
 * @details Covers input reading, product-catalog helpers, IVA table access,
 * EAN validation, monetary rounding, wildcard matching, formatted output,
 * invoice-client argument parsing, and client-record lookup.
 */

#ifndef UTILS_H
#define UTILS_H

#include "common.h"

/* ── Input ──────────────────────────────────────────────────────────────── */

/**
 * @brief Read the rest of the current @c stdin line as a single token.
 *
 * @details Leading and trailing whitespace is stripped. If the line exceeds
 * ::MAX_INSTRC_LENGTH bytes the buffer is filled and the remainder is drained
 * so @c stdin stays synchronised with subsequent commands.
 *
 * @param sys System state (used for safemalloc()).
 * @return Heap-allocated NUL-terminated string, or @c NULL for a blank line.
 *         Caller must free with free_safe().
 */
char *read_token_safe(SystemState *sys);

/* ── Product catalog helpers ─────────────────────────────────────────────── */

/**
 * @brief @c bsearch comparator: EAN string key vs ::Product element.
 * @param key  Pointer to a @c const @c char* EAN string.
 * @param elem Pointer to a ::Product entry.
 * @return Negative, zero, or positive per @c strcmp semantics.
 */
int cmp_product_search(const void *key, const void *elem);

/**
 * @brief Binary-search the catalog for a product by EAN.
 * @param sys System state.
 * @param ean NUL-terminated EAN string to find.
 * @return Zero-based index into @c sys->catalog, or @c -1 if not found.
 */
int find_product_idx(SystemState *sys, const char *ean);

/* ── IVA helpers ─────────────────────────────────────────────────────────── */

/**
 * @brief Check whether an IVA class letter is defined in the table.
 * @param table  IVA rate table of ::IVA_TABLE_SIZE entries.
 * @param letter Class letter (@c 'A'–@c 'Z').
 * @return Non-zero if the slot is present, zero otherwise.
 */
int iva_is_present(const Iva table[], char letter);

/**
 * @brief Retrieve the tax rate for an IVA class letter.
 * @param table  IVA rate table of ::IVA_TABLE_SIZE entries.
 * @param letter Class letter (@c 'A'–@c 'Z').
 * @return Tax percentage, or @c 0 if the letter is not present.
 */
int get_iva_rate(const Iva table[], char letter);

/**
 * @brief Define or update one slot in the IVA rate table.
 * @param table  IVA rate table of ::IVA_TABLE_SIZE entries.
 * @param letter Class letter (@c 'A'–@c 'Z'); silently ignored otherwise.
 * @param value  Tax percentage to store.
 */
void iva_set(Iva table[], char letter, int value);

/* ── Validation ──────────────────────────────────────────────────────────── */

/**
 * @brief Validate an EAN-8 or EAN-13 code including its check digit.
 * @param ean NUL-terminated digit string (exactly 8 or 13 characters).
 * @return Non-zero if valid, zero otherwise.
 */
int validate_ean(const char *ean);

/**
 * @brief Check that a description starts with a valid first byte.
 *
 * @details Valid first bytes are ASCII uppercase letters (@c 'A'–@c 'Z') or
 * the UTF-8 lead byte of a Latin-supplement uppercase letter (≥ @c 0xC0).
 *
 * @param s NUL-terminated description string.
 * @return Non-zero if valid, zero otherwise.
 */
int is_valid_desc_start(const char *s);

/**
 * @brief Check that a client name starts with a letter.
 *
 * @details Accepts ASCII letters (@c 'A'–@c 'Z', @c 'a'–@c 'z') and the
 * UTF-8 lead byte for Latin-supplement characters (≥ @c 0xC0).
 * Rejects strings that begin with a digit.
 *
 * @param s NUL-terminated name string.
 * @return Non-zero if valid, zero otherwise.
 */
int is_valid_name_start(const char *s);

/**
 * @brief Validate all fields of a @c p command in priority order.
 *
 * @details Prints the first error found and returns zero. The priority is:
 * EAN → IVA → price → quantity → description.
 *
 * @param ean    EAN string (already truncated to 13 chars).
 * @param iva_ok Non-zero if the IVA class is present in the table.
 * @param price  Parsed price; use @c -1.0 as sentinel for unparseable input.
 * @param stock  Parsed quantity; use @c -1 as sentinel for unparseable input.
 * @param desc   Description string, or @c NULL if absent.
 * @return Non-zero if all fields are valid, zero if an error was printed.
 */
int validate_p_input(const char *ean, int iva_ok, double price, int stock,
                     const char *desc);

/* ── Math ────────────────────────────────────────────────────────────────── */

/**
 * @brief Round @p val to the nearest cent using symmetric (half-up) rounding.
 * @param val Monetary value in currency units.
 * @return Value rounded to two decimal places.
 */
double round_money(double val);

/* ── Pattern matching ────────────────────────────────────────────────────── */

/**
 * @brief Test whether @p text matches a shell-style @p pattern.
 *
 * @details Supports @c '*' (any sequence of characters) and @c '?' (any
 * single character). Matching is case-sensitive and operates on raw bytes.
 *
 * @param pattern Wildcard pattern string.
 * @param text    Text to match against the pattern.
 * @return Non-zero if @p text matches @p pattern, zero otherwise.
 */
int match(const char *pattern, const char *text);

/* ── Output ──────────────────────────────────────────────────────────────── */

/**
 * @brief Print one product line in @c cmd_l format.
 *
 * @details Format: @c "<ean> <iva> <price> <sold> <stock> <desc>"
 *
 * @param p Pointer to the product to print.
 */
void print_product(const Product *p);

/**
 * @brief Print one basket line in @c cmd_a format.
 *
 * @details Format: @c "<iva> <price> <qty> <total-with-iva> <desc>"
 *
 * @param sys     System state (catalog is read for price/IVA/description).
 * @param table   IVA rate table.
 * @param cat_idx Index into @c sys->catalog for the product.
 * @param qty     Quantity to display (may be zero).
 */
void print_basket_item(SystemState *sys, const Iva table[], int cat_idx,
                       int qty);

/* ── Parsing ─────────────────────────────────────────────────────────────── */

/**
 * @brief Parse the argument string of an @c f command into NIF and name.
 *
 * @details The optional leading integer is treated as the NIF only when it is
 * followed by whitespace or end-of-string. A leading zero causes @c *nif to
 * be set to @c 0 (failing the range check in @c cmd_f) so the raw digit
 * string can be preserved for the error message. An unclosed quote leaves
 * @p name_buf empty as an invalidity signal.
 *
 * @param line          NUL-terminated argument string (after the @c f command).
 * @param[out] nif      Receives the parsed NIF, or its initial value if absent.
 * @param[out] name_buf Caller-supplied buffer that receives the client name.
 * @param name_buf_size Size of @p name_buf in bytes.
 */
int parse_invoice_client(const char *line, int *nif, char *name_buf,
                         size_t name_buf_size);

/* ── Name comparison ─────────────────────────────────────────────────────── */

/**
 * @brief Compare two client name strings for sorted-array ordering.
 *
 * @details Currently delegates to @c strcmp (byte order), matching the
 * ordering expected by the test suite. Both get_or_create_client() insertion
 * sort and cmp_client_search() @c bsearch must use this function so that the
 * sorted invariant remains consistent.
 *
 * @param a First name string.
 * @param b Second name string.
 * @return Negative, zero, or positive per @c strcmp semantics.
 */
int cmp_names(const char *a, const char *b);

/* ── Client-record lookup ────────────────────────────────────────────────── */

/**
 * @brief Binary-search the client array for a record by name.
 * @param sys  System state.
 * @param name NUL-terminated client name to find.
 * @return Zero-based index into @c sys->clients, or @c -1 if not found.
 */
int find_client_idx(SystemState *sys, const char *name);

#endif /* UTILS_H */