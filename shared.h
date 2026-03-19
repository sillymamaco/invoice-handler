/**
 * @file shared.h
 * @brief Cross-file prototypes shared across basket.c, invoice.c, catalog.c,
 * and commands.c. Not part of the public API; do not include from main.c.
 */

#ifndef SHARED_H
#define SHARED_H

#include "common.h"

/** @brief Return all basket quantities to stock and reset basket_count to 0.
 *  @param sys System state. */
void cancel_basket(SystemState *sys);

/** @brief Print all basket items sorted by EAN ascending.
 *  @param sys System state. @param table IVA rate table. */
void print_sorted_basket(SystemState *sys, Iva table[]);

/** @brief Add (positive qty) or remove (negative qty) units from the basket,
 *  printing the updated basket line. Zero-quantity slots are kept so the
 *  product-in-use guard in cmd_p still fires.
 *  @param sys System state. @param table IVA rate table.
 *  @param ean EAN of the product. @param qty Units to add or remove. */
void process_basket_add(SystemState *sys, Iva table[], const char *ean,
                        int qty);

/** @brief Return the index of a ClientRecord, inserting a new sorted entry
 *  if none exists for @p name.
 *  @param sys System state. @param name Client name. @param nif Client NIF. */
int get_or_create_client(SystemState *sys, const char *name, int nif);

/** @brief Close the basket, append an invoice to the client's queue, update
 *  global counters, and print the invoice summary line.
 *  @param sys System state. @param table IVA rate table.
 *  @param nif Client NIF. @param name Client name. */
void finalize_invoice(SystemState *sys, Iva table[], int nif,
                      const char *name);

/** @brief Delete the invoice with the given ID, adjust global counters, and
 *  print its summary. Prints an error if the ID is not found.
 *  @param sys System state. @param inv_id Invoice ID to delete. */
void cmd_d_delete_inv(SystemState *sys, int inv_id);

/** @brief Reduce product stock by @p qty, removing the product from the
 *  catalog when stock reaches zero. Blocked if basket reservation would be
 *  violated.
 *  @param sys System state. @param ean Product EAN. @param qty Units to remove. */
void cmd_d_reduce_stock(SystemState *sys, const char *ean, int qty);

/** @brief Drain all remaining characters on the current stdin line. */
void drain_line(void);

/** @brief Read one p command argument line from stdin into @p buf, skipping
 *  leading whitespace. Returns non-zero if at least one character was read.
 *  @param buf Destination buffer. @param bufsz Size of buf in bytes. */
int read_p_line(char *buf, int bufsz);

/** @brief Return a heap-allocated copy of the product description from a p
 *  command line (everything after the first four tokens), or NULL if empty.
 *  @param line Full argument line. @param sys System state. */
char *extract_desc(const char *line, SystemState *sys);

/** @brief Insert a new product into the catalog keeping it sorted by EAN.
 *  Ownership of @p desc is transferred.
 *  @param sys System state. @param ean EAN string. @param iva_c IVA letter.
 *  @param price Unit price. @param stock Initial stock. @param desc Description. */
void catalog_insert(SystemState *sys, const char *ean, char iva_c,
                    double price, int stock, char *desc);

/** @brief Update an existing product. Rejects with "product in use" if the
 *  price changes while the product is in the basket. Returns non-zero on
 *  success.
 *  @param sys System state. @param idx Catalog index. @param ean Product EAN.
 *  @param iva_c New IVA letter. @param price New price. @param stock Stock to add.
 *  @param desc New description (ownership transferred on success). */
int catalog_update(SystemState *sys, int idx, const char *ean,
                   char iva_c, double price, int stock, char *desc);

/** @brief Parse EAN, IVA, price, and quantity from a p command line, using
 *  sentinel values (-1.0 / -1) for unparseable fields to enforce correct
 *  error priority. Returns non-zero when four tokens were found.
 *  @param linebuf Full argument line. @param ean Output buffer (14 bytes).
 *  @param iva_c Receives IVA letter or '\\0' for multi-char tokens.
 *  @param price Receives price or -1.0. @param stock Receives quantity or -1. */
int parse_p_fields(const char *linebuf, char ean[14], char *iva_c,
                   double *price, int *stock);

/** @brief Allocate and return a Product* array sorted by insert_order for
 *  cmd_l. Returns NULL when the catalog is empty. Caller must free with
 *  free_safe(ordered, catalog_count * sizeof(Product*), sys).
 *  @param sys System state. */
Product **build_ordered(SystemState *sys);

/** @brief Print in-stock products matching one EAN token or wildcard pattern
 *  in insertion order. Returns non-zero if anything was printed.
 *  @param sys System state. @param ordered Insertion-order pointer array.
 *  @param token EAN string or wildcard pattern. */
int print_l_token(SystemState *sys, Product **ordered, const char *token);

/** @brief Print all in-stock products in insertion order (bare l command).
 *  Prints "*: no such product" when nothing is available.
 *  @param sys System state. @param ordered Insertion-order pointer array. */
void cmd_l_all(SystemState *sys, Product **ordered);

/** @brief Print products for each whitespace-separated token in @p buf,
 *  printing "<token>: no such product" for unmatched tokens.
 *  @param sys System state. @param ordered Insertion-order pointer array.
 *  @param buf Mutable token string modified in place by strtok. */
void cmd_l_tokens(SystemState *sys, Product **ordered, char *buf);

#endif /* SHARED_H */