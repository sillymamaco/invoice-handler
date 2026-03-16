/**
 * @file catalog.h
 * @brief Product catalog management using binary search.
 * @author ist1117890
 */
#ifndef CATALOG_H
#define CATALOG_H

#include "types.h"

/**
 * @brief Comparison function used by bsearch to find a product by EAN.
 * @param key Pointer to the target EAN string.
 * @param elem Pointer to the Product struct.
 * @return Integer indicating lexicographical order.
 */
int cmp_product_search(const void *key, const void *elem);

/**
 * @brief Finds the index of a product in the catalog using binary search.
 * @param sys Pointer to the system state.
 * @param ean EAN string to find.
 * @return Index of the product, or -1 if not found.
 */
int find_product_idx(SystemState *sys, const char *ean);

/**
 * @brief Prints the details of a given product.
 * @param p Pointer to the product to print.
 */
void print_product(Product *p);

/**
 * @brief Inserts a new product maintaining the array sorted by EAN.
 * @param sys Pointer to the system state.
 * @param ean Product EAN.
 * @param iva_c VAT class character.
 * @param price Product price.
 * @param stock Initial product stock.
 * @param desc Product description.
 * @return The index where the product was inserted.
 */
int insert_product_sorted(SystemState *sys, const char *ean, char iva_c,
                          double price, int stock, char *desc);

/**
 * @brief Reduces the stock of a product or deletes it if stock reaches zero.
 * @param sys Pointer to the system state.
 * @param ean EAN of the product.
 * @param qty Amount to reduce.
 */
void cmd_d_reduce_stock(SystemState *sys, const char *ean, int qty);

#endif