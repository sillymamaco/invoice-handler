#include "catalog.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int cmp_product_search(const void *key, const void *elem) {
  return strcmp((const char *)key, ((const Product *)elem)->ean);
}

int find_product_idx(SystemState *sys, const char *ean) {
  if (sys->catalog_count == 0)
    return -1;
  Product *p = bsearch(ean, sys->catalog, sys->catalog_count, sizeof(Product),
                       cmp_product_search);
  if (p)
    return (int)(p - sys->catalog);
  return -1;
}

void print_product(Product *p) {
  printf("%s %c %.2f %d %d %s\n", p->ean, p->iva_class, p->price, p->sold,
         p->stock, p->desc);
}

int insert_product_sorted(SystemState *sys, const char *ean, char iva_c,
                          double price, int stock, char *desc) {
  if (sys->catalog_count == sys->catalog_capacity) {
    int new_cap = sys->catalog_capacity ? sys->catalog_capacity * 2 : BASE_CAP;
    sys->catalog =
        safe_realloc(sys->catalog, sys->catalog_capacity * sizeof(Product),
                     new_cap * sizeof(Product), sys);
    sys->catalog_capacity = new_cap;
  }
  int idx = sys->catalog_count - 1;
  while (idx >= 0 && strcmp(sys->catalog[idx].ean, ean) > 0) {
    sys->catalog[idx + 1] = sys->catalog[idx];
    idx--;
  }
  idx++;
  strcpy(sys->catalog[idx].ean, ean);
  sys->catalog[idx].iva_class = iva_c;
  sys->catalog[idx].price = price;
  sys->catalog[idx].stock = stock;
  sys->catalog[idx].desc = desc;
  sys->catalog[idx].sold = 0;
  sys->catalog_count++;
  return idx;
}

void cmd_d_reduce_stock(SystemState *sys, const char *ean, int qty) {
  int cat_idx = find_product_idx(sys, ean);
  if (cat_idx == -1) {
    printf("%s: no such product\n", ean);
    return;
  }
  int will_delete = (sys->catalog[cat_idx].stock <= qty);
  int in_basket = 0;
  for (int j = 0; j < sys->basket_count; j++) {
    if (strcmp(sys->basket[j].ean, ean) == 0) {
      in_basket = 1;
      break;
    }
  }
  if (will_delete && in_basket) {
    printf("product in use\n");
    return;
  }
  if (qty <= 0 || qty > sys->catalog[cat_idx].stock) {
    printf("invalid quantity\n");
    return;
  }
  sys->catalog[cat_idx].stock -= qty;
  if (sys->catalog[cat_idx].stock == 0) {
    printf("0 %s\n", sys->catalog[cat_idx].desc);
    size_t len = strlen(sys->catalog[cat_idx].desc) + 1;
    free_safe(sys->catalog[cat_idx].desc, len, sys);
    for (int j = cat_idx; j < sys->catalog_count - 1; j++)
      sys->catalog[j] = sys->catalog[j + 1];
    sys->catalog_count--;
  } else {
    printf("%d %s\n", sys->catalog[cat_idx].stock, sys->catalog[cat_idx].desc);
  }
}