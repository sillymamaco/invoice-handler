/**
 * @file common.h
 * @author IST1117890 (Irina Cojocari)
 * @brief Global type definitions and system state for the billing system.
 */

#ifndef COMMON_H
#define COMMON_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** @brief Maximum heap memory the program may use (10 MiB). */
#define MAX_MEMORY_ALLOCATED (10 * 1024 * 1024)

/** @brief Maximum length of a single input line in bytes. */
#define MAX_INSTRC_LENGTH 65535

/** @brief Multi-purpose buffer for strings. */
#define BUFFER_LIMIT 1024

/** @brief Number of slots in the IVA lookup table. */
#define IVA_TABLE_SIZE 26

/** @brief Maximum catalog products allowed. */
#define MAX_CATALOG_PRODUCTS 10000

/** @brief Default NIF value for final consumer. */
#define DEFAULT_NIF 999999999

/**
 * @brief One entry in the IVA rate table.
 */
typedef struct {
  int value;   /**< Tax percentage, e.g. 23 for 23 %. */
  int present; /**< Non-zero when this slot has been explicitly defined. */
} Iva;

/**
 * @brief A product registered in the catalog.
 */
typedef struct {
  char *desc;       /**< Heap-allocated description string. */
  double price;     /**< Unit price before IVA. */
  int stock;        /**< Units currently available. */
  int sold;         /**< Cumulative units sold or reserved. */
  int insert_order; /**< Counter used in cmd_l. */
  char ean[14];     /**< EAN-8 or EAN-13 code plus '\0' */
  char iva_class;   /**< IVA class letter, e.g. 'D'. */
} Product;

/**
 * @brief A finalised invoice stored in a client's FIFO queue.
 */
typedef struct {
  long long total_cents; /**< Total value including IVA. */
  int nif;               /**< Client NIF. */
  int id;                /**< Invoice creation ID. */
  int num_items;         /**< Number of product units covered. */
} Invoice;

/**
 * @brief Record per-client that owns a FIFO queue of invoices.
 */
typedef struct {
  char *name;        /**< Heap-allocated client name. */
  int nif;           /**< Client NIF. */
  Invoice *invoices; /**< Dynamic array of invoices. */
  int invoice_count; /**< Number of invoices currently stored. */
  int invoice_cap;   /**< Allocated capacity of the invoices array. */
} ClientRecord;

/**
 * @brief One line item in the shopping basket.
 */
typedef struct {
  char ean[14]; /**< EAN code that identifies the product. */
  int amount;   /**< Quantity in the basket. */
} BasketItem;

/**
 * @brief Top-level system state passed to every command handler.
 */
typedef struct {
  Product *catalog;     /**< Dynamic array of products sorted by EAN. */
  int catalog_count;    /**< Number of products currently in the catalog. */
  int catalog_capacity; /**< Allocated capacity of the catalog array. */

  ClientRecord *clients; /**< Dynamic array of client records. */
  int client_count;      /**< Number of client records stored. */
  int client_capacity;   /**< Allocated capacity of the clients array. */

  BasketItem *basket;  /**< Dynamic array of current basket line items. */
  int basket_count;    /**< Number of distinct products in the basket. */
  int basket_capacity; /**< Allocated capacity of the basket array. */

  size_t memory_used; /**< Running total of heap bytes currently tracked. */

  int next_invoice_id;    /**< Next invoice ID to assign. */
  int next_product_order; /**< Counter of product insertion order. */
  int global_items;       /**< Total item count across all live invoices. */
  long long global_sales_cents; /**< Total revenue across all invoices. */
} SystemState;

#endif /* COMMON_H */