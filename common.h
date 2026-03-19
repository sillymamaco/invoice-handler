/**
 * @file common.h
 * @brief Global type definitions and system state for the billing system.
 *
 * @details
 * Two key design decisions shape the data layout:
 *
 * **IVA table** — a fixed array of ::IVA_TABLE_SIZE slots indexed by
 * @c (letter - 'A'), giving O(1) lookup and alphabetical iteration.
 * A slot is active only when its @c present flag is non-zero.
 *
 * **Invoice history** — per-client FIFO queues. ::ClientRecord entries live
 * in a dynamic array sorted by name (enabling @c bsearch). Each record owns
 * a dynamic @c Invoice array in strict chronological order: index 0 is the
 * oldest invoice, index @c n-1 the newest.
 */

#ifndef COMMON_H
#define COMMON_H

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** @brief Maximum heap memory the program may use (8 MiB). */
#define MAX_MEMORY_ALLOCATED (8 * 1024 * 1024)

/** @brief Maximum length of a single input line in bytes (64 KiB). */
#define MAX_INSTRC_LENGTH 65535

/** @brief General-purpose line-buffer size in bytes. */
#define BUFFER_LIMIT 1024

/** @brief Number of slots in the IVA lookup table (one per letter A–Z). */
#define IVA_TABLE_SIZE 26

/**
 * @brief One entry in the IVA rate table.
 *
 * @details The table is a fixed array of ::IVA_TABLE_SIZE entries.
 * Slot @c i corresponds to class letter @c ('A' + i).
 * A slot is valid only when @c present is non-zero.
 */
typedef struct {
  int value;   /**< Tax percentage, e.g. @c 23 for 23 %. */
  int present; /**< Non-zero when this slot has been explicitly defined. */
} Iva;

/**
 * @brief A product registered in the catalog.
 *
 * @details The catalog is a dynamic array sorted by EAN, enabling O(log n)
 * lookups via @c bsearch. The ::insert_order field records the value of the
 * monotonic counter ::SystemState::next_product_order at insertion time so
 * that @c cmd_l can reproduce creation order independently of EAN order.
 */
typedef struct {
  char *desc;       /**< Heap-allocated description string (max 50 bytes). */
  double price;     /**< Unit price before IVA; always positive. */
  int stock;        /**< Units currently available (not in basket). */
  int sold;         /**< Cumulative units sold or reserved in the basket. */
  int insert_order; /**< Monotonic counter snapshot used for @c cmd_l order. */
  char ean[14];     /**< EAN-8 or EAN-13 code, NUL-terminated. */
  char iva_class;   /**< IVA class letter, e.g. @c 'D'. */
} Product;

/**
 * @brief A finalised invoice stored in a client's FIFO queue.
 */
typedef struct {
  double total;  /**< Total value including IVA. */
  int nif;       /**< Client NIF (tax identification number). */
  int id;        /**< Unique, monotonically increasing invoice identifier. */
  int num_items; /**< Number of product units covered by this invoice. */
} Invoice;

/**
 * @brief Per-client record that owns a FIFO queue of invoices.
 *
 * @details The @c clients array inside ::SystemState is sorted by @c name,
 * enabling O(log n) lookups via @c bsearch. The @c invoices array is
 * append-only, so chronological order is preserved without extra sorting.
 */
typedef struct {
  char *name;        /**< Heap-allocated, NUL-terminated client name. */
  int nif;           /**< Client NIF stored for printing with @c cmd_d. */
  Invoice *invoices; /**< Dynamic array of invoices in chronological order. */
  int invoice_count; /**< Number of invoices currently stored. */
  int invoice_cap;   /**< Allocated capacity of the @c invoices array. */
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
 * @details All dynamic arrays are grown with safe_realloc and released
 * with clean_all(). This struct is the single root of all heap allocations;
 * no global variables are used.
 */
typedef struct {
  Product *catalog;      /**< Dynamic array of products sorted by EAN. */
  int catalog_count;     /**< Number of products currently in the catalog. */
  int catalog_capacity;  /**< Allocated capacity of the @c catalog array. */

  ClientRecord *clients; /**< Dynamic array of client records sorted by name. */
  int client_count;      /**< Number of client records stored. */
  int client_capacity;   /**< Allocated capacity of the @c clients array. */

  BasketItem *basket;    /**< Dynamic array of current basket line items. */
  int basket_count;      /**< Number of distinct products in the basket. */
  int basket_capacity;   /**< Allocated capacity of the @c basket array. */

  size_t memory_used;    /**< Running total of heap bytes currently tracked. */

  int next_invoice_id;    /**< Next invoice ID to assign; starts at 1. */
  int next_product_order; /**< Monotonic counter for product insertion order. */
  int global_items;       /**< Total item count across all live invoices. */
  double global_sales;    /**< Total revenue across all live invoices. */
} SystemState;

#endif /* COMMON_H */