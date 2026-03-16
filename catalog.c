#include "catalog.h"
#include "memory.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void swap_product(Product *a, Product *b) {
  Product temp = *a;
  *a = *b;
  *b = temp;
}

int partition_catalog(Product *arr, int low, int high) {
  char *pivot = arr[high].ean;
  int i = (low - 1);
  for (int j = low; j < high; j++) {
    if (strcmp(arr[j].ean, pivot) < 0) {
      i++;
      swap_product(&arr[i], &arr[j]);
    }
  }
  swap_product(&arr[i + 1], &arr[high]);
  return (i + 1);
}

void sort_catalog(Product *arr, int low, int high) {
  if (low < high) {
    int pi = partition_catalog(arr, low, high);
    sort_catalog(arr, low, pi - 1);
    sort_catalog(arr, pi + 1, high);
  }
}

int cmp_product_search(const void *key, const void *elem) {
  return strcmp((const char *)key, ((const Product *)elem)->ean);
}

int find_product_idx(SystemState *sys, const char *ean) {
    for (int i = 0; i < sys->catalog_count; i++)
        if (strcmp(sys->catalog[i].ean, ean) == 0)
            return i;
    return -1;
}

void print_product(Product *p) {
  printf("%s %c %.2f %d %d %s\n", p->ean, p->iva_class, p->price, p->sold,
         p->stock, p->desc);
}

void cmd_d_reduce_stock(SystemState *sys, const char *ean, int qty) {
  int cat_idx = find_product_idx(sys, ean);
  if (cat_idx == -1) {
    printf("%s: no such product\n", ean);
    return;
  }

  if (qty <= 0 || qty > sys->catalog[cat_idx].stock) {
    printf("invalid quantity\n");
    return;
  }

  int will_delete = (sys->catalog[cat_idx].stock == qty);
  if (will_delete) {
    for (int j = 0; j < sys->basket_count; j++) {
      if (strcmp(sys->basket[j].ean, ean) == 0) {
        printf("product in use\n");
        return;
      }
    }
  }

  sys->catalog[cat_idx].stock -= qty;
  if (sys->catalog[cat_idx].stock == 0) {
    printf("0 %s\n", sys->catalog[cat_idx].desc);
    free_safe(sys->catalog[cat_idx].desc, strlen(sys->catalog[cat_idx].desc) + 1, sys);
    
    // Shift para a esquerda para manter o catálogo contíguo e ordenado
    for (int j = cat_idx; j < sys->catalog_count - 1; j++) {
      sys->catalog[j] = sys->catalog[j + 1];
    }
    sys->catalog_count--;
  } else {
    printf("%d %s\n", sys->catalog[cat_idx].stock, sys->catalog[cat_idx].desc);
  }
}