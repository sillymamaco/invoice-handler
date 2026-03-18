/**
 * @file internal.h
 * @brief Internal prototypes shared across the billing system source files.
 *
 * @details These functions are not part of the public API and must not be
 * included by external code. They are declared here (without @c static) so
 * that @c basket.c, @c invoice.c, @c catalog.c, and @c commands.c can each
 * be compiled as separate translation units while still calling each other.
 */

#ifndef INTERNAL_H
#define INTERNAL_H

#include "common.h"

/* ── basket.c ──────────────────────────────────────────────────────────── */

/**
 * @brief Return all basket quantities to stock and reset the basket.
 * @param sys System state.
 */
void cancel_basket(SystemState *sys);

/**
 * @brief Print all basket items sorted by EAN ascending.
 * @param sys   System state.
 * @param table IVA rate table.
 */
void print_sorted_basket(SystemState *sys, Iva table[]);

/**
 * @brief Add or remove units of a product from the basket.
 * @param sys   System state.
 * @param table IVA rate table.
 * @param ean   EAN of the product.
 * @param qty   Units to add (positive) or remove (negative).
 */
void process_basket_add(SystemState *sys, Iva table[], const char *ean,
                        int qty);

/* ── invoice.c ─────────────────────────────────────────────────────────── */

/**
 * @brief Return the index of a ::ClientRecord, creating one if absent.
 * @param sys  System state.
 * @param name Client name.
 * @param nif  Client NIF (used only when creating a new record).
 * @return Zero-based index into @c sys->clients.
 */
int get_or_create_client(SystemState *sys, const char *name, int nif);

/**
 * @brief Close the basket, create an invoice, and update global counters.
 * @param sys   System state.
 * @param table IVA rate table.
 * @param nif   Client NIF.
 * @param name  Client name.
 */
void finalize_invoice(SystemState *sys, Iva table[], int nif, const char *name);

/**
 * @brief Delete the invoice identified by @p inv_id.
 * @param sys    System state.
 * @param inv_id Invoice identifier to delete.
 */
void cmd_d_delete_inv(SystemState *sys, int inv_id);

/**
 * @brief Reduce the stock of a product, removing it from the catalog when
 *        the stock reaches zero.
 * @param sys System state.
 * @param ean EAN of the product.
 * @param qty Number of units to remove.
 */
void cmd_d_reduce_stock(SystemState *sys, const char *ean, int qty);

/* ── catalog.c ─────────────────────────────────────────────────────────── */

/**
 * @brief Drain all remaining characters on the current @c stdin line.
 */
void drain_line(void);

/**
 * @brief Read one argument line for the @c p command from @c stdin.
 * @param buf   Destination buffer.
 * @param bufsz Size of @p buf in bytes.
 * @return Non-zero if at least one character was read, zero otherwise.
 */
int read_p_line(char *buf, int bufsz);

/**
 * @brief Extract the product description from a @c p command line.
 * @param line Full argument line.
 * @param sys  System state.
 * @return Heap-allocated description string, or @c NULL.
 */
char *extract_desc(const char *line, SystemState *sys);

/**
 * @brief Insert a new product into the catalog, keeping it sorted by EAN.
 * @param sys   System state.
 * @param ean   NUL-terminated EAN string.
 * @param iva_c IVA class letter.
 * @param price Unit price.
 * @param stock Initial stock quantity.
 * @param desc  Heap-allocated description (ownership transferred).
 */
void catalog_insert(SystemState *sys, const char *ean, char iva_c, double price,
                    int stock, char *desc);

/**
 * @brief Update an existing product's fields in the catalog.
 * @param sys   System state.
 * @param idx   Index into @c sys->catalog.
 * @param ean   EAN (for basket membership check).
 * @param iva_c New IVA class letter.
 * @param price New unit price.
 * @param stock Additional stock to add.
 * @param desc  New heap-allocated description (ownership transferred on
 * success).
 * @return Non-zero on success, zero if blocked by @c product-in-use.
 */
int catalog_update(SystemState *sys, int idx, const char *ean, char iva_c,
                   double price, int stock, char *desc);

/**
 * @brief Parse the four fixed fields of a @c p command line.
 * @param linebuf  Full argument line.
 * @param ean      Output buffer (14 bytes) for the EAN string.
 * @param iva_c    Receives the IVA class letter, or @c '\\0' for multi-char
 * tokens.
 * @param price    Receives the parsed price, or @c -1.0 on failure.
 * @param stock    Receives the parsed quantity, or @c -1 on failure.
 * @return Non-zero when four tokens were found, zero otherwise.
 */
int parse_p_fields(const char *linebuf, char ean[14], char *iva_c,
                   double *price, int *stock);

/**
 * @brief Allocate a @c Product* array sorted by ::Product::insert_order.
 * @param sys System state.
 * @return Heap-allocated pointer array, or @c NULL when catalog is empty.
 */
Product **build_ordered(SystemState *sys);

/**
 * @brief Print in-stock products matching one EAN token (exact or wildcard).
 * @param sys     System state.
 * @param ordered Product pointer array sorted by insertion order.
 * @param token   EAN string or wildcard pattern.
 * @return Non-zero if at least one product was printed.
 */
int print_l_token(SystemState *sys, Product **ordered, const char *token);

/**
 * @brief Print all in-stock products in insertion order (bare @c l).
 * @param sys     System state.
 * @param ordered Product pointer array sorted by insertion order.
 */
void cmd_l_all(SystemState *sys, Product **ordered);

/**
 * @brief Print products for each whitespace-separated token in @p buf.
 * @param sys     System state.
 * @param ordered Product pointer array sorted by insertion order.
 * @param buf     Mutable token string.
 */
void cmd_l_tokens(SystemState *sys, Product **ordered, char *buf);

#endif /* INTERNAL_H */