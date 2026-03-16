/**
 * @file types.h
 * @author ist1117890
 * @brief Core data structures and memory constants.
 */
#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

/** Maximum memory allocated for the program */
#define MAX_MEMORY_ALLOCATED (4 * 1024 * 1024)

/** Maximum length for an instruction line */
#define MAX_INSTRC_LENGTH 65535

/** Standard buffer limit for smaller strings */
#define BUFFER_LIMIT 1024

/**
 * @brief Structure representing a VAT (IVA) tier.
 */
typedef struct {
  int value;
  char letter;
} Iva;

/**
 * @brief Structure representing a product in the catalog.
 */
typedef struct {
  char *desc;
  double price;
  int stock;
  int sold;
  char ean[14];
  char iva_class;
} Product;

/**
 * @brief Structure representing a client invoice.
 */
typedef struct {
  char *client_name;
  double total;
  int nif;
  int id;
  int num_items;
} Invoice;

/**
 * @brief Structure representing an item inside the current basket.
 */
typedef struct {
  char ean[14];
  int amount;
} BasketItem;

/**
 * @brief Structure representing the global state of the system.
 */
typedef struct {
  Product *catalog;
  int catalog_count;
  int catalog_capacity;
  Invoice *history;
  int history_count;
  int history_capacity;
  BasketItem *basket;
  int basket_count;
  int basket_capacity;
  int memory_used;
  int next_invoice_id;
  int global_items;
  double global_sales;
} SystemState;

#endif