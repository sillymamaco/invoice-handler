/**
 * @file types.h
 * @brief Core data structures and system constants.
 * @author ist1117890
 */
#ifndef TYPES_H
#define TYPES_H

#include <stddef.h>
#include <stdint.h>

/** @brief Maximum memory allowed to be allocated (4MB). */
#define MAX_MEM_ALLOC (4 * 1024 * 1024)

/** @brief Maximum length for an instruction/string buffer. */
#define MAX_INST_LEN 65535

/** @brief Standard buffer limit for smaller strings. */
#define BUF_LIMIT 1024

/** @brief Maximum size for the EAN string including null-terminator. */
#define EAN_SIZE 14

/** @brief Valid length for an 8-digit EAN. */
#define EAN_LEN_8 8

/** @brief Valid length for a 13-digit EAN. */
#define EAN_LEN_13 13

/** @brief Maximum allowed length for a product description. */
#define MAX_DESC_LEN 50

/** @brief Minimum valid NIF value. */
#define MIN_NIF 100000000

/** @brief Maximum valid NIF value. */
#define MAX_NIF 999999999

/** @brief Base capacity for dynamic array allocations. */
#define BASE_CAP 10

/** @brief Offset used for rounding monetary values. */
#define ROUND_OFFSET 0.500000001

/** @brief Divisor to convert percentages to decimals. */
#define PCT_DIV 100.0

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
  char ean[EAN_SIZE];
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
  char ean[EAN_SIZE];
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