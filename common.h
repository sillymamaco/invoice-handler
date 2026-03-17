/**
 * @file common.h
 * @brief Global definitions and data structures for the system.
 */

#ifndef COMMON_H
#define COMMON_H

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Constants to ensure the memory doesnt blow up and avoid magic numbers*/
#define MAX_MEMORY_ALLOCATED (4 * 1024 * 1024)
#define MAX_INSTRC_LENGTH 65535
#define BUFFER_LIMIT 1024

/**
 * @struct Iva
 * @brief Represents a VAT (IVA) class.
 */
typedef struct {
  int value;    /**< Percentage value */
  char letter;  /**< Identifier letter */
} Iva;

/**
 * @struct Product
 * @brief Represents a product in the catalog.
 */
typedef struct {
  char *desc;        /**< Product description */
  double price;      /**< Unit price */
  int stock;         /**< Current stock quantity */
  int sold;          /**< Total quantity sold */
  int insert_order;  /**< Order of insertion for stable sorting */
  char ean[14];      /**< EAN-13 or EAN-8 barcode */
  char iva_class;    /**< VAT class identifier */
} Product;

/**
 * @struct Invoice
 * @brief Represents a finalized invoice.
 */
typedef struct {
  char *client_name; /**< Client name */
  double total;      /**< Total invoice value */
  int nif;           /**< Client NIF (Tax ID) */
  int id;            /**< Invoice ID */
  int num_items;     /**< Total number of items */
} Invoice;

/**
 * @struct BasketItem
 * @brief Represents an item in the current shopping basket.
 */
typedef struct {
  char ean[14];      /**< Product barcode */
  int amount;        /**< Quantity to purchase */
} BasketItem;

/**
 * @struct SystemState
 * @brief Holds the entire state of the application.
 */
typedef struct {
  Product *catalog;        /**< Array of products */
  int catalog_count;       /**< Current number of products */
  int catalog_capacity;    /**< Allocated capacity for products */
  Invoice *history;        /**< Array of invoices */
  int history_count;       /**< Current number of invoices */
  int history_capacity;    /**< Allocated capacity for invoices */
  BasketItem *basket;      /**< Array of basket items */
  int basket_count;        /**< Current number of basket items */
  int basket_capacity;     /**< Allocated capacity for basket items */
  size_t memory_used;      /**< Total memory used in bytes */
  int next_invoice_id;     /**< Next available invoice ID */
  int global_items;        /**< Global count of sold items */
  double global_sales;     /**< Global sum of all sales */
} SystemState;

#endif