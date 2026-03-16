/**
 * @file basket.h
 * @brief Active basket operations and sorting.
 * @author ist1117890
 */
#ifndef BASKET_H
#define BASKET_H

#include "types.h"

/**
 * @brief Prints a specific item from the basket.
 * @param sys Pointer to the system state.
 * @param table Array of VAT classes.
 * @param iva_count Total VAT count.
 * @param cat_idx Catalog index of the product.
 * @param qty Amount added to the basket.
 */
void print_basket_item(SystemState *sys, Iva table[], int iva_count,
                       int cat_idx, int qty);

/**
 * @brief Restores the stock of products in the current basket and clears it.
 * @param sys Pointer to the system state.
 */
void cancel_basket(SystemState *sys);

/**
 * @brief Swaps two elements in the basket array.
 * @param a Pointer to the first element.
 * @param b Pointer to the second element.
 */
void swap_basket(BasketItem *a, BasketItem *b);

/**
 * @brief Partitions the basket array for Quicksort.
 * @param arr Array of BasketItem.
 * @param low Starting index.
 * @param high Ending index.
 * @return The partition index.
 */
int partition_basket(BasketItem *arr, int low, int high);

/**
 * @brief Sorts the basket using the Quicksort algorithm.
 * @param arr Array of BasketItem.
 * @param low Starting index.
 * @param high Ending index.
 */
void quick_sort_basket(BasketItem *arr, int low, int high);

/**
 * @brief Prints all items currently in the basket sorted by EAN.
 * @param sys Pointer to the system state.
 * @param table Array of VAT classes.
 * @param iva_count Total VAT count.
 */
void print_sorted_basket(SystemState *sys, Iva table[], int iva_count);

/**
 * @brief Adds a quantity of a specific product to the basket.
 * @param sys Pointer to the system state.
 * @param table Array of VAT classes.
 * @param iva_count Total VAT count.
 * @param ean EAN of the product.
 * @param qty Quantity to add.
 */
void process_basket_add(SystemState *sys, Iva table[], int iva_count,
                        const char *ean, int qty);

#endif