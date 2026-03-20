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

/** @brief Maximum heap memory the program may use (64 MiB). */
#define MAX_MEMORY_ALLOCATED (64 * 1024 * 1024)

/** @brief Maximum length of a single input line in bytes (64 KiB). */
#define MAX_INSTRC_LENGTH 65535

/** @brief Multi-purpose buffer for strings. */
#define BUFFER_LIMIT 1024

/** @brief Number of slots in the IVA lookup table (one per letter A–Z). */
#define IVA_TABLE_SIZE 26

/**
 * @brief One entry in the IVA rate table.
 *
 * @details The IVA table is an array of IVA_TABLE_SIZE entries. Each slot
 * corresponds to a letter (Index is get by subtracting 'A' to the iva class)
 * and is a struct as follows: its value and whether that cell is filled.
 */
typedef struct {
  int value;   /**< Tax percentage, e.g. 23 for 23 %. */
  int present; /**< Non-zero when this slot has been explicitly defined. */
} Iva;

/**
 * @brief A product registered in the catalog.
 *
 * @details The catalog is a dynamic array sorted by EAN (to have O log n
 * lookups with bsearch). The insertion_order is kept so that in cmd_l, the
 * products can be listed by creation order.
 */
typedef struct {
  char *desc;       /**< Heap-allocated description string (max 50 bytes). */
  double price;     /**< Unit price before IVA; always positive. */
  int stock;        /**< Units currently available (not in basket). */
  int sold;         /**< Cumulative units sold or reserved in the basket. */
  int insert_order; /**< Counter used in cmd_l. */
  char ean[14];     /**< EAN-8 or EAN-13 code plus '\0' */
  char iva_class;   /**< IVA class letter, e.g. 'D'. */
} Product;

/**
 * @brief A finalised invoice stored in a client's FIFO queue.
 * @details The invoices are stored in an array, each new client being
 * inserted in the right position, by alphabetical order. Each client is a
 * FIFO pile of invoices, ensuring they are in chronological order.
 */
typedef struct {
  double total;  /**< Total value including IVA. */
  int nif;       /**< Client NIF (tax identification number). */
  int id;        /**< Number in the order of invoice creation. */
  int num_items; /**< Number of product units covered by this invoice. */
} Invoice;

/**
 * @brief Record per-client that owns a FIFO queue of invoices.
 *
 * @details The clients in the array in SystemState are sorted by name,
 * allowing O log n lookup using bsearch. The array or invoices is append-only,
 * to ensure they are in chronological order and dont need sorting.
 */
typedef struct {
  char *name;        /**< Heap-allocated client name. */
  int nif;           /**< Client NIF stored for printing with cmd_d. */
  Invoice *invoices; /**< Dynamic array of invoices in chronological order. */
  int invoice_count; /**< Number of invoices currently stored. */
  int invoice_cap;   /**< Allocated capacity of the invoices array. */
} ClientRecord;

/**
 * @brief One line item in the shopping basket.
 */
typedef struct {
  char ean[14]; /**< EAN code that identifies the product. */
  int amount;   /**< Quantity in the basket; may reach zero after removals. */
} BasketItem;

/**
 * @brief Top-level system state passed to every command handler.
 *
 * @details This struct is the holy grail of the program. All hail SystemState.
 * keeps track of max capacities (that change because i use dynamic arrays),
 * and currently_used capacities and also keeps the pointers to said arrays.
 * Keeps the number of invoices printed, number of products added, total sales.
 * and total items sold. Keeps track of the memory that is dynamically used to
 * ensure i dont run out.
 */
typedef struct {
  Product *catalog;     /**< Dynamic array of products sorted by EAN. */
  int catalog_count;    /**< Number of products currently in the catalog. */
  int catalog_capacity; /**< Allocated capacity of the catalog array. */

  ClientRecord *clients; /**< Dynamic array of client records sorted by name.*/
  int client_count;      /**< Number of client records stored. */
  int client_capacity;   /**< Allocated capacity of the clients array. */

  BasketItem *basket;  /**< Dynamic array of current basket line items. */
  int basket_count;    /**< Number of distinct products in the basket. */
  int basket_capacity; /**< Allocated capacity of the basket array. */

  size_t memory_used; /**< Running total of heap bytes currently tracked. */

  int next_invoice_id;    /**< Next invoice ID to assign; starts at 1. */
  int next_product_order; /**< Counter of product insertion order.*/
  int global_items;       /**< Total item count across all live invoices. */
  double global_sales;    /**< Total revenue across all live invoices. */
} SystemState;

#endif /* COMMON_H */
