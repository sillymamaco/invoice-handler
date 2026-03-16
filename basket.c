#include "basket.h"
#include "catalog.h"
#include "memory.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

void print_basket_item(SystemState *sys, Iva table[], int iva_count,
                       int cat_idx, int qty) {
  double price = sys->catalog[cat_idx].price;
  int iva = get_iva_rate(table, iva_count, sys->catalog[cat_idx].iva_class);
  double total = (price * qty) * (1.0 + (iva / 100.0));
  printf("%c %.2f %d %.2f %s\n", sys->catalog[cat_idx].iva_class, price, qty,
         round_money(total), sys->catalog[cat_idx].desc);
}

void cancel_basket(SystemState *sys) {
  for (int i = 0; i < sys->basket_count; i++) {
    int cat_idx = find_product_idx(sys, sys->basket[i].ean);
    if (cat_idx != -1) {
      sys->catalog[cat_idx].stock += sys->basket[i].amount;
      sys->catalog[cat_idx].sold -= sys->basket[i].amount;
    }
  }
  sys->basket_count = 0;
}

void swap_basket(BasketItem *a, BasketItem *b) {
  BasketItem temp = *a;
  *a = *b;
  *b = temp;
}

int partition_basket(BasketItem *arr, int low, int high) {
  char *pivot = arr[high].ean;
  int i = (low - 1);
  for (int j = low; j < high; j++) {
    if (strcmp(arr[j].ean, pivot) < 0) {
      i++;
      swap_basket(&arr[i], &arr[j]);
    }
  }
  swap_basket(&arr[i + 1], &arr[high]);
  return (i + 1);
}

void sort_basket(BasketItem *arr, int low, int high) {
  if (low < high) {
    int pi = partition_basket(arr, low, high);
    sort_basket(arr, low, pi - 1);
    sort_basket(arr, pi + 1, high);
  }
}

void print_sorted_basket(SystemState *sys, Iva table[], int iva_count) {
  if (sys->basket_count > 1)
    sort_basket(sys->basket, 0, sys->basket_count - 1);
  for (int i = 0; i < sys->basket_count; i++) {
    int cat_idx = find_product_idx(sys, sys->basket[i].ean);
    if (cat_idx != -1)
      print_basket_item(sys, table, iva_count, cat_idx, sys->basket[i].amount);
  }
}

void process_basket_add(SystemState *sys, Iva table[], int iva_count,
                        const char *ean, int qty) {
  int cat_idx = find_product_idx(sys, ean), basket_idx = -1;
  for (int j = 0; j < sys->basket_count; j++) {
    if (strcmp(sys->basket[j].ean, ean) == 0) {
      basket_idx = j;
      break;
    }
  }

  if (cat_idx == -1) {
    printf("%s: no such product\n", ean);
    return;
  }
  if (qty > sys->catalog[cat_idx].stock ||
      (qty < 0 &&
       (basket_idx == -1 || sys->basket[basket_idx].amount + qty < 0))) {
    printf("no stock\n");
    return;
  }

  sys->catalog[cat_idx].stock -= qty;
  sys->catalog[cat_idx].sold += qty;
  if (basket_idx != -1) {
    sys->basket[basket_idx].amount += qty;
  } else {
    if (sys->basket_count == sys->basket_capacity) {
      int new_cap = sys->basket_capacity ? sys->basket_capacity * 2 : 10;
      sys->basket =
          safe_realloc(sys->basket, sys->basket_capacity * sizeof(BasketItem),
                       new_cap * sizeof(BasketItem), sys);
      sys->basket_capacity = new_cap;
    }
    basket_idx = sys->basket_count++;
    strcpy(sys->basket[basket_idx].ean, ean);
    sys->basket[basket_idx].amount = qty;
  }
  print_basket_item(sys, table, iva_count, cat_idx,
                    sys->basket[basket_idx].amount);
}