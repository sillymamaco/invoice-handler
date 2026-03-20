/*
 * @file shared.h
 * @author IST1117890 (Irina Cojocari)
 * @brief Cross-file prototypes shared across basket.c, invoice.c, catalog.c,
 * and commands.c. Not part of the public API; not included in main.c.
 */

#ifndef SHARED_H
#define SHARED_H

#include "common.h"

/*
 * @brief Return all basket quantities to stock and reset basket_count to 0.
 *  @param System state.
 */
void cancel_basket(SystemState *sys);

/*
 * @brief Print all basket items sorted by EAN ascending.
 * @param System state.
 * @param table IVA rate table.
 */
void print_sorted_basket(SystemState *sys, Iva table[]);

/*
 * @brief Add (positive qty) or remove (negative qty) units from the basket,
 *  printing the updated basket line. Zero-quantity slots are kept so the
 *  product-in-use guard in cmd_p still fires.
 * @param System state.
 * @param IVA rate table.
 * @param EAN of the product.
 * @param Units to add or remove.
 */
void process_basket_add(SystemState *sys, Iva table[], const char *ean,
                        int qty);

/* @brief Return the index of a ClientRecord, inserting a new sorted entry
 *  if none exists for name.
 * @param System state.
 * @param Client name.
 * @param Client NIF.
 */
int get_or_create_client(SystemState *sys, const char *name, int nif);

/* @brief Close the basket, append an invoice to the client's queue, update
 *  global counters, and print the invoice summary line.
 * @param System state.
 * @param IVA rate table.
 * @param Client NIF.
 * @param Client name.
 */
void finalize_invoice(SystemState *sys, Iva table[], int nif, const char *name);

/* @brief Delete the invoice with the given ID, adjust global counters, and
 *  print its summary. Prints an error if the ID is not found.
 * @param System state.
 * @param Invoice ID to delete.
 */
void cmd_d_delete_inv(SystemState *sys, int inv_id);

/* @brief Reduce product stock by qty, removing the product from the
 *  catalog when stock reaches zero. Blocked if basket reservation would be
 *  violated.
 * @param System state.
 * @param Product EAN.
 * @param Units to remove.
 */
void cmd_d_reduce_stock(SystemState *sys, const char *ean, int qty);

/* @brief Drain all remaining characters on the current stdin line. */
void drain_line(void);

/* @brief Read one p command argument line from stdin into buf, skipping
 *  leading whitespace. Returns non-zero if at least one character was read.
 * @param Destination buffer.
 * @param Size of buf in bytes.
 */
int read_p_line(char *buf, int bufsz);

/* @brief Return a heap-allocated copy of the product description from a p
 *  command line (everything after the first four tokens), or NULL if empty.
 * @param Full argument line.
 * @param System state.
 */
char *extract_desc(const char *line, SystemState *sys);

/* @brief Insert a new product into the catalog keeping it sorted by EAN.
 *  Ownership of desc is transferred.
 * @param System state.
 * @param EAN string.
 * @param IVA letter.
 * @param Unit price.
 * @param stock Initial stock.
 * @param desc Description.
 */
void catalog_insert(SystemState *sys, const char *ean, char iva_c, double price,
                    int stock, char *desc);

/* @brief Update an existing product. Rejects with "product in use" if the
 *  price changes while the product is in the basket. Returns non-zero on
 *  success.
 * @param System state.
 * @param Catalog index.
 * @param Product EAN.
 * @param New IVA letter.
 * @param New price.
 * @param Stock to add.
 * @param New description (ownership transferred on success).
 */
int catalog_update(SystemState *sys, int idx, const char *ean, char iva_c,
                   double price, int stock, char *desc);

/* @brief Parse EAN, IVA, price, and quantity from a p command line, using
 *  sentinel values (-1.0 / -1) for unparseable fields to enforce correct
 *  error priority. Returns non-zero when four tokens were found.
 * @param Full argument line.
 * @param Output buffer (14 bytes).
 * @param Receives IVA letter or '\\0' for multi-char tokens.
 * @param Receives price or -1.0.
 * @param Receives quantity or -1.
 */
int parse_p_fields(const char *linebuf, char ean[14], char *iva_c,
                   double *price, int *stock);

/* @brief Allocate and return a Product* array sorted by insert_order for
 *  cmd_l. Returns NULL when the catalog is empty. Caller must free with
 *  free_safe(ordered, catalog_count * sizeof(Product*), sys).
 * @param System state.
 */
Product **build_ordered(SystemState *sys);

/* @brief Print in-stock products matching one EAN token or wildcard pattern
 *  in insertion order. Returns non-zero if anything was printed.
 * @param System state.
 * @param Insertion-order pointer array.
 * @param EAN string or wildcard pattern.
 */
int print_l_token(SystemState *sys, Product **ordered, const char *token);

/* @brief Print all in-stock products in insertion order (bare l command).
 *  Prints "*: no such product" when nothing is available.
 * @param System state.
 * @param Insertion-order pointer array.
 */
void cmd_l_all(SystemState *sys, Product **ordered);

/* @brief Print products for each whitespace-separated token in buf,
 *  printing "<token>: no such product" for unmatched tokens.
 * @param System state.
 * @param Insertion-order pointer array.
 * @param Mutable token string modified in place by strtok.
 */
void cmd_l_tokens(SystemState *sys, Product **ordered, char *buf);

#endif /* SHARED_H */
