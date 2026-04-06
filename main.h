/**
 * Global types and system state for the billing system.
 * @file main.h
 * @author IST1117890 (Irina Cojocari)
 */

#ifndef MAIN_H
#define MAIN_H

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Maximum length of a single input line in bytes. */
#define MAX_INSTRC_LENGTH 65535

/* Multi-purpose buffer for strings. */
#define BUFFER_LIMIT 1024

/* Number of slots in the IVA lookup table. */
#define IVA_TABLE_SIZE 26

/* Maximum catalog products allowed. */
#define MAX_CATALOG_PRODUCTS 10000

/* Default NIF value for final consumer. */
#define DEFAULT_NIF 999999999

/*
 * Entry per IVA in the IVA table (a fixed array of ivas).
 */
typedef struct {
  int value;   /*< Tax percentage, e.g. 23 for 23 %. */
  int present; /*< Non-zero when this slot has been explicitly defined. */
} Iva;

/*
 * Entry per product in the catalog (dinamic array of products by EAN).
 */
typedef struct {
  char *desc;       /*< Dinamically allocated description string. */
  double price;     /*< Unit price before IVA. */
  int stock;        /*< Units currently available. */
  int sold;         /*< Cumulative units sold or in the basket. */
  int insert_order; /*< Counter used in cmd_l. */
  char ean[14];     /*< EAN-8 or EAN-13 code plus '\0' */
  char iva_class;   /*< IVA class letter, e.g. 'D'. */
} Product;

/*
 * Entry per invoice in the dynamic array for each Client (FIFO). Each client
 * is stored in a dynamic array that sorts clients by nif (a hash table if you
 * will).
 */
typedef struct {
  long long total_cents; /*< Total value including IVA. */
  int nif;               /*< Client NIF. */
  int id;                /*< Invoice creation ID. */
  int num_items;         /*< Number of product units covered. */
} Invoice;

/*
 * Entry per client in the dynamic array sorted by nif. Each client owns a
 * FIFO array of invoices.
 */
typedef struct {
  char *name;        /*< Dinamically allocated client name. */
  int nif;           /*< Client NIF. */
  Invoice *invoices; /*< Dynamic array of invoices. */
  int invoice_count; /*< Number of invoices currently stored. */
  int invoice_cap;   /*< Allocated capacity of the invoices array. */
} ClientRecord;

/*
 * Entry per item in the shopping basket.
 */
typedef struct {
  char ean[14]; /*< EAN code that identifies the product. */
  int amount;   /*< Quantity in the basket. */
} BasketItem;

/*
 * Struct passed to every command so that everything comes together.
 */
typedef struct {
  Product *catalog;     /*< Dynamic array of products sorted by EAN. */
  int catalog_count;    /*< Number of products currently in the catalog. */
  int catalog_capacity; /*< Allocated capacity of the catalog array. */

  ClientRecord *clients; /*< Dynamic array of client records. */
  int client_count;      /*< Number of client records stored. */
  int client_capacity;   /*< Allocated capacity of the clients array. */

  BasketItem *basket;  /*< Dynamic array of current basket line items. */
  int basket_count;    /*< Number of distinct products in the basket. */
  int basket_capacity; /*< Allocated capacity of the basket array. */

  size_t memory_used; /*< Running total of bytes dynamically allocated. */

  int next_invoice_id;    /*< Next invoice ID to assign. */
  int next_product_order; /*< Counter of product insertion order. */
  int global_items;       /*< Total item count across all live invoices. */
  long long global_sales_cents; /*< Total revenue across all invoices. */
} SystemState;

#endif /* MAIN_H */
