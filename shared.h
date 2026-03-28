/**
 * Cross-file prototypes shared across internal modules.
 * @file shared.h
 * @author IST1117890 (Irina Cojocari)
 */

#ifndef SHARED_H
#define SHARED_H

#include "common.h"

/**
 * Return all basket quantities to stock and reset basket_count to 0.
 * @param sys System state.
 */
void cancel_basket(SystemState *sys);

/**
 * Print all basket items sorted by EAN ascending.
 * @param sys System state.
 * @param table IVA rate table.
 */
void print_sorted_basket(SystemState *sys, Iva table[]);

/**
 * Add or remove units from the basket.
 * @param sys System state.
 * @param table IVA rate table.
 * @param ean EAN of the product.
 * @param qty Units to add or remove.
 */
void process_basket_add(SystemState *sys, Iva table[], const char *ean,
                        int qty);

/**
 * Return the index of a ClientRecord, inserting a new one if needed.
 * @param sys System state.
 * @param name Client name.
 * @param nif Client NIF.
 * @return Index of the client.
 */
int get_or_create_client(SystemState *sys, const char *name, int nif);

/**
 * Close the basket and append an invoice to the client's queue.
 * @param sys System state.
 * @param table IVA rate table.
 * @param nif Client NIF.
 * @param name Client name.
 */
void finalize_invoice(SystemState *sys, Iva table[], int nif, const char *name);

/**
 * Delete the invoice with the given ID.
 * @param sys System state.
 * @param inv_id Invoice ID to delete.
 */
void cmd_d_delete_inv(SystemState *sys, int inv_id);

/**
 * Reduce product stock by qty.
 * @param sys System state.
 * @param ean Product EAN.
 * @param qty Units to remove.
 */
void cmd_d_reduce_stock(SystemState *sys, const char *ean, int qty);

/**
 * Read one command argument line from stdin into buf.
 * @param buf Destination buffer.
 * @param bufsz Size of buf in bytes.
 * @return Non-zero if characters were read.
 */
int read_p_line(char *buf, int bufsz);

/**
 * Return a dynamically allocated copy of the product description.
 * @param line Full argument line.
 * @param sys System state.
 * @return Description string or NULL.
 */
char *extract_desc(const char *line, SystemState *sys);

/**
 * Insert a new product into the catalog.
 * @param sys System state.
 * @param ean EAN string.
 * @param iva_c IVA letter.
 * @param price Unit price.
 * @param stock Initial stock.
 * @param desc Description.
 */
void catalog_insert(SystemState *sys, const char *ean, char iva_c, double price,
                    int stock, char *desc);

/**
 * Update an existing product.
 * @param sys System state.
 * @param idx Catalog index.
 * @param ean Product EAN.
 * @param iva_c New IVA letter.
 * @param price New price.
 * @param stock Stock to add.
 * @param desc New description.
 * @return Non-zero on success.
 */
int catalog_update(SystemState *sys, int idx, const char *ean, char iva_c,
                   double price, int stock, char *desc);

/**
 * Parse fields from a p command line.
 * @param linebuf Full argument line.
 * @param ean Output buffer for EAN.
 * @param iva_c Receives IVA letter.
 * @param price Receives price.
 * @param stock Receives quantity.
 * @return Non-zero when tokens are successfully parsed.
 */
int parse_p_fields(const char *linebuf, char ean[14], char *iva_c,
                   double *price, int *stock);

/**
 * Allocate and return a Product* array sorted by insert_order.
 * @param sys System state.
 * @return Sorted array of Product pointers.
 */
Product **build_ordered(SystemState *sys);

/**
 * Print in-stock products matching a token.
 * @param sys System state.
 * @param ordered Insertion-order pointer array.
 * @param token EAN string or wildcard pattern.
 * @return Non-zero if anything was printed.
 */
int print_l_token(SystemState *sys, Product **ordered, const char *token);

/**
 * Print all in-stock products in insertion order.
 * @param sys System state.
 * @param ordered Insertion-order pointer array.
 */
void cmd_l_all(SystemState *sys, Product **ordered);

/**
 * Print products for each whitespace-separated token.
 * @param sys System state.
 * @param ordered Insertion-order pointer array.
 * @param buf Mutable token string.
 */
void cmd_l_tokens(SystemState *sys, Product **ordered, char *buf);

#endif /* SHARED_H */
