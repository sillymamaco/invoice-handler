/**
 * @file utils.h
 * @author IST1117890 (Irina Cojocari)
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
 * @brief Read the rest of the current stdin line as a single token.
 *
 * @details Leading and trailing whitespace is stripped. If the line exceeds
 * MAX_INSTRC_LENGTH bytes the buffer is filled and the remainder is drained
 * so stdin stays synchronised with subsequent commands.
 *
 * @param sys System state (used for safemalloc()).
 * @return Dynamically allocated string, or NULL for a blank line.
 *         Caller must free with free_safe().
 */
char *read_token_safe(SystemState *sys);

/* ── Product catalog helpers ─────────────────────────────────────────────── */

/**
 * @brief bsearch comparator: EAN string key vs Product element.
 * @param Pointer to a EAN string.
 * @param Pointer to a product in the array.
 * @return Negative, zero, or positive per strcmp semantics.
 */
int cmp_product_search(const void *key, const void *elem);

/**
 * @brief Binary-search the catalog for a product by EAN.
 * @param sys System state.
 * @param EAN string to find.
 * @return Zero-based index into sys->catalog, or -1 if not found.
 */
int find_product_idx(SystemState *sys, const char *ean);

/* ── IVA helpers ─────────────────────────────────────────────────────────── */

/**
 * @brief Check whether an IVA class letter is defined in the table.
 * @param IVA rate table of IVA_TABLE_SIZE entries.
 * @param Class letter ('A' - 'Z').
 * @return Non-zero if the slot is present, zero otherwise.
 */
int iva_is_present(const Iva table[], char letter);

/**
 * @brief Retrieve the tax rate for an IVA class letter.
 * @param IVA rate table of IVA_TABLE_SIZE entries.
 * @param Class letter ('A'– 'Z').
 * @return Tax percentage, or 0 if the letter is not present.
 */
int get_iva_rate(const Iva table[], char letter);

/**
 * @brief Define or update one slot in the IVA rate table.
 * @param IVA rate table of ::IVA_TABLE_SIZE entries.
 * @param Class letter ('A'– 'Z'); silently ignored otherwise.
 * @param Tax percentage to store.
 */
void iva_set(Iva table[], char letter, int value);

/* ── Validation ──────────────────────────────────────────────────────────── */

/**
 * @brief Validate an EAN-8 or EAN-13 code including its check digit.
 * @param EAN string (exactly 8 or 13 characters).
 * @return Non-zero if valid, zero otherwise.
 */
int validate_ean(const char *ean);

/**
 * @brief Check that a description starts with a valid first byte.
 *
 * @details Valid first bytes are ASCII uppercase letters ('A'–'Z') or
 * the UTF-8 lead byte of a Latin-supplement uppercase letter (>=  0xC0).
 *
 * @param Description string.
 * @return Non-zero if valid, zero otherwise.
 */
int is_valid_desc_start(const char *s);

/**
 * @brief Check that a client name starts with a letter.
 *
 * @details Accepts ASCII letters ('A'– 'Z', 'a'– 'z') and the
 * UTF-8 lead byte for Latin-supplement characters (>= 0xC0).
 * Rejects strings that begin with a digit.
 *
 * @param s NUL-terminated name string.
 * @return Non-zero if valid, zero otherwise.
 */
int is_valid_name_start(const char *s);

/**
 * @brief Validate all fields of a p command in priority order.
 *
 * @details Prints the first error found and returns zero. The priority is:
 * EAN → IVA → price → quantity → description.
 *
 * @param EAN string (already truncated to 13 chars).
 * @param Non-zero if the IVA class is present in the table.
 * @param Parsed price; use -1.0 as sentinel for unparseable input.
 * @param Parsed quantity; use -1 as sentinel for unparseable input.
 * @param Description string, or NULL if absent.
 * @return Non-zero if all fields are valid, zero if an error was printed.
 */
int validate_p_input(const char *ean, int iva_ok, double price, int stock,
                     const char *desc);

/* ── Math ────────────────────────────────────────────────────────────────── */

/**
 * @brief Round val to the nearest cent using symmetric (half-up) rounding.
 * @param Monetary value in currency units.
 * @return Value rounded to two decimal places.
 */
double round_money(double val);

/* ── Pattern matching ────────────────────────────────────────────────────── */

/**
 * @brief Test whether text matches a shell-style pattern.
 *
 * @details Supports '*' (any sequence of characters) and '?' (any
 * single character). Matching is case-sensitive and operates on raw bytes.
 *
 * @param Wildcard pattern string.
 * @param Text to match against the pattern.
 * @return Non-zero if text matches pattern, zero otherwise.
 */
int match(const char *pattern, const char *text);

/* ── Output ──────────────────────────────────────────────────────────────── */

/**
 * @brief Print one product line in cmd_l format.
 *
 * @details Format: "<ean> <iva> <price> <sold> <stock> <desc>"
 *
 * @param Pointer to the product to print.
 */
void print_product(const Product *p);

/**
 * @brief Print one basket line in cmd_a format.
 *
 * @details Format: "<iva> <price> <qty> <total-with-iva> <desc>"
 *
 * @param System state (catalog is read for price/IVA/description).
 * @param IVA rate table.
 * @param cat_idx -> Index of the product in the sys->catalog.
 * @param Quantity to display (may be zero).
 */
void print_basket_item(SystemState *sys, const Iva table[], int cat_idx,
                       int qty);

/* ── Parsing ─────────────────────────────────────────────────────────────── */

/**
 * @brief Parse the argument string of an f command into NIF and name.
 *
 * @details The optional leading integer is treated as the NIF only when it is
 * followed by whitespace or end-of-string. A leading zero causes *nif to
 * be set to 0 (failing the range check in cmd_f) so the raw digit
 * string can be preserved for the error message. An unclosed quote leaves
 * name_buf empty as an invalidity signal.
 *
 * @param Argument string (after the f command).
 * @param Receives the parsed NIF, or its initial value if absent.
 * @param Caller-supplied buffer that receives the client name.
 * @param Size of name_buf in bytes.
 */
int parse_invoice_client(const char *line, int *nif, char *name_buf,
                         size_t name_buf_size);

/* ── Name comparison ─────────────────────────────────────────────────────── */

/**
 * @brief Compare two client name strings for sorted-array ordering.
 *
 * @details Used in cmp_client_search() bsearch and get_or_create_client()
 * insertion sort as a key. Compares a and b.
 *
 * @param First name string.
 * @param Second name string.
 * @return Negative, zero, or positive per strcmp semantics.
 */
int cmp_names(const char *a, const char *b);

/* ── Client-record lookup ────────────────────────────────────────────────── */

/**
 * @brief Binary-search the client array for a record by name.
 * @param System state.
 * @param Client name to find.
 * @return Zero-based index into sys->clients, or -1 if not found.
 */
int find_client_idx(SystemState *sys, const char *name);

#endif /* UTILS_H */
