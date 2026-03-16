/**
 * @file catalog.h
 * @author ist1117890
 * @brief Product catalog operations.
 */
#ifndef CATALOG_H
#define CATALOG_H

#include "types.h"
#include <stdint.h>

/**
 * @brief Finds a product index in the catalog using hash and EAN.
 * @param sys Pointer to system state.
 * @param ean The EAN to search for.
 * @param h The precalculated hash of the EAN.
 * @return The index of the product, or -1 if not found.
 */
int find_product_idx(SystemState *sys, const char *ean);

/**
 * @brief Prints the details of a given product.
 * @param p Pointer to the product to print.
 */
void print_product(Product *p);

/**
 * @brief Reduces the stock of a product or deletes it if stock reaches zero.
 * @param sys Pointer to the system state.
 * @param ean EAN of the product.
 * @param qty Amount to reduce.
 */
void cmd_d_reduce_stock(SystemState *sys, const char *ean, int qty);

/**
 * @brief Sorts the catalog using quicksort.
 * @param arr Pointer to the array or items.
 * @param low Beginning of the array.
 * @param high End of array.
 */
void sort_catalog(Product *arr, int low, int high)
#endif