/**
 * @file utils.c
 * @brief Utility functions for input parsing, validation, and math.
 */

#include "utils.h"
#include "memory.h"

char *read_token_safe(SystemState *sys) {
  char buf[MAX_INSTRC_LENGTH] = {0};
  int c, i = 0;
  while ((c = getchar()) != '\n' && c != EOF && isspace(c));
  if (c != '\n' && c != EOF) {
    buf[i++] = (char)c;
    while ((c = getchar()) != '\n' && c != EOF)
      if (i < MAX_INSTRC_LENGTH - 1 && c != '\r')
        buf[i++] = (char)c;
  }
  while (i > 0 && isspace((unsigned char)buf[i - 1]))
    i--;
  buf[i] = '\0';
  if (i == 0) return NULL;
  char *res = safemalloc(i + 1, sys);
  if (res) strcpy(res, buf);
  return res;
}

int cmp_product_search(const void *key, const void *elem) {
  return strcmp((const char *)key, ((const Product *)elem)->ean);
}

int cmp_insert_order(const void *a, const void *b) {
  const Product *pa = *(const Product **)a;
  const Product *pb = *(const Product **)b;
  return pa->insert_order - pb->insert_order;
}

int find_product_idx(SystemState *sys, const char *ean) {
  if (sys->catalog_count == 0) return -1;
  Product *p = bsearch(ean, sys->catalog, sys->catalog_count, sizeof(Product), cmp_product_search);
  return p ? (int)(p - sys->catalog) : -1;
}

int get_iva_rate(Iva table[], int iva_count, char iva_class) {
  for (int i = 0; i < iva_count; i++)
    if (table[i].letter == iva_class) return table[i].value;
  return 0;
}

int validate_ean(const char *ean) {
  int sum = 0, len = (int)strlen(ean);
  if (len != 8 && len != 13) return 0;
  for (int i = 0; i < len - 1; i++) {
    int val = ean[i] - '0';
    sum += (i % 2 == 0) ? val : 3 * val;
  }
  return (((10 - (sum % 10)) % 10) == (ean[len - 1] - '0'));
}

int match(const char *pattern, const char *text) {
  const char *star = NULL, *ts = text;
  while (*text) {
    if (*pattern == '?' || *pattern == *text) { pattern++; text++; }
    else if (*pattern == '*') { star = pattern++; ts = text; }
    else if (star) { pattern = star + 1; text = ++ts; }
    else return 0;
  }
  while (*pattern == '*') pattern++;
  return *pattern == '\0';
}

int is_valid_desc_start(const char *s) {
  if (!s || !s[0]) return 0;
  unsigned char c = (unsigned char)s[0];
  if (c >= 'A' && c <= 'Z') return 1;
  if (c >= 0xC0) return 1;
  return 0;
}

int is_valid_name_start(const char *s) {
  if (!s || !s[0]) return 0;
  unsigned char c = (unsigned char)s[0];
  if (isdigit(c)) return 0;
  if (c >= 'A' && c <= 'Z') return 1;
  if (c >= 'a' && c <= 'z') return 1;
  if (c >= 0xC0) return 1;
  return 0;
}

double round_money(double val) {
  return (long long)(val * 100.0 + 0.500000001) / 100.0;
}

void print_product(const Product *p) {
  printf("%s %c %.2f %d %d %s\n", p->ean, p->iva_class, p->price, p->sold, p->stock, p->desc);
}

void print_basket_item(SystemState *sys, Iva table[], int iva_count, int cat_idx, int qty) {
  double price = sys->catalog[cat_idx].price;
  int iva = get_iva_rate(table, iva_count, sys->catalog[cat_idx].iva_class);
  double total = (price * qty) * (1.0 + (iva / 100.0));
  printf("%c %.2f %d %.2f %s\n", sys->catalog[cat_idx].iva_class, price, qty, round_money(total), sys->catalog[cat_idx].desc);
}

int validate_p_input(const char *ean, int iva_ok, double price, int stock, const char *desc) {
  int desc_valid = desc && is_valid_desc_start(desc) && strlen(desc) <= 50;
  if (!validate_ean(ean)) { printf("invalid ean\n"); return 0; }
  if (!iva_ok) { printf("invalid iva\n"); return 0; }
  if (price <= 0) { printf("invalid price\n"); return 0; }
  if (stock < 0) { printf("invalid quantity\n"); return 0; }
  if (!desc || !desc_valid) { printf("invalid description\n"); return 0; }
  return 1;
}

void parse_invoice_client(char *line, int *nif, char **name) {
  char *ptr = line;
  while (*ptr && isspace(*ptr)) ptr++;
  if (isdigit(*ptr)) {
    sscanf(ptr, "%d", nif);
    while (*ptr && !isspace(*ptr)) ptr++;
  }
  while (*ptr && isspace(*ptr)) ptr++;
  if (*ptr) {
    if (*ptr == '"') {
      ptr++;
      char *end = strchr(ptr, '"');
      if (end) { *end = '\0'; *name = ptr; }
      else { *name = "1_invalid"; }
    } else {
      *name = ptr;
      char *end = *name;
      while (*end && !isspace((unsigned char)*end)) end++;
      *end = '\0';
    }
  }
}