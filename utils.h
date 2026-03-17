/**
 * @file utils.h
 * @brief Utility function prototypes for validation, math, and parsing.
 */

#ifndef UTILS_H
#define UTILS_H

#include "common.h"

char *read_token_safe(SystemState *sys);
int cmp_product_search(const void *key, const void *elem);
int cmp_insert_order(const void *a, const void *b);
int find_product_idx(SystemState *sys, const char *ean);
int get_iva_rate(Iva table[], int iva_count, char iva_class);
int validate_ean(const char *ean);
int match(const char *pattern, const char *text);
int is_valid_desc_start(const char *s);
int is_valid_name_start(const char *s);
double round_money(double val);
void print_product(const Product *p);
void print_basket_item(SystemState *sys, Iva table[], int iva_count, int cat_idx, int qty);
int validate_p_input(const char *ean, int iva_ok, double price, int stock, const char *desc);
void parse_invoice_client(char *line, int *nif, char **name);

#endif