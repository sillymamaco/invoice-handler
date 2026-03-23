/**
 * @file basket.c
 * @author IST1117890 (Irina Cojocari)
 * @brief Shopping basket operations.
 */

#include "commands.h"
#include "memory.h"
#include "shared.h"
#include "utils.h"

void cancel_basket(SystemState *sys) {
  for (int i = 0; i < sys->basket_count; i++) {
    int idx = find_product_idx(sys, sys->basket[i].ean);
    if (idx != -1) {
      sys->catalog[idx].stock += sys->basket[i].amount;
      sys->catalog[idx].sold -= sys->basket[i].amount;
      if (sys->catalog[idx].sold < 0) {
        sys->catalog[idx].sold = 0;
      }
    }
  }
  sys->basket_count = 0;
}

static void swap_basket(BasketItem *a, BasketItem *b) {
  BasketItem t = *a;
  *a = *b;
  *b = t;
}

static int partition_basket(BasketItem *arr, int lo, int hi) {
  char *pivot = arr[hi].ean;
  int i = lo - 1;
  for (int j = lo; j < hi; j++) {
    if (strcmp(arr[j].ean, pivot) < 0) {
      i++;
      swap_basket(&arr[i], &arr[j]);
    }
  }
  swap_basket(&arr[i + 1], &arr[hi]);
  return i + 1;
}

static void quick_sort_basket(BasketItem *arr, int lo, int hi) {
  if (lo < hi) {
    int pi = partition_basket(arr, lo, hi);
    quick_sort_basket(arr, lo, pi - 1);
    quick_sort_basket(arr, pi + 1, hi);
  }
}

void print_sorted_basket(SystemState *sys, Iva table[]) {
  if (sys->basket_count > 1) {
    quick_sort_basket(sys->basket, 0, sys->basket_count - 1);
  }
  for (int i = 0; i < sys->basket_count; i++) {
    if (sys->basket[i].amount > 0) {
      int idx = find_product_idx(sys, sys->basket[i].ean);
      if (idx != -1) {
        print_basket_item(sys, table, idx, sys->basket[i].amount);
      }
    }
  }
}

/**
 * @brief Handles allocation and insertion of a new basket item.
 * @param sys System state.
 * @param ean EAN to insert.
 * @param new_amount Quantity.
 * @return Index of the new basket item.
 */
static int insert_new_basket_item(SystemState *sys, const char *ean,
                                  int new_amount) {
  if (sys->basket_count == sys->basket_capacity) {
    int nc = sys->basket_capacity ? sys->basket_capacity * 2 : 10;
    sys->basket =
        safe_realloc(sys->basket, sys->basket_capacity * sizeof(BasketItem),
                     nc * sizeof(BasketItem), sys);
    sys->basket_capacity = nc;
  }
  int idx = sys->basket_count++;
  strcpy(sys->basket[idx].ean, ean);
  sys->basket[idx].amount = new_amount;
  return idx;
}

static int get_basket_idx(SystemState *sys, const char *ean) {
  for (int j = 0; j < sys->basket_count; j++) {
    if (strcmp(sys->basket[j].ean, ean) == 0) {
      return j;
    }
  }
  return -1;
}

/**
 * @brief Validates if an item can be added to or removed from the basket.
 * @param sys System state.
 * @param ean EAN string.
 * @param qty Quantity to adjust.
 * @param b_idx Output pointer for basket index.
 * @param c_idx Output pointer for catalog index.
 * @return Non-zero if valid.
 */
static int validate_basket_add(SystemState *sys, const char *ean, int qty,
                               int *b_idx, int *c_idx) {
  if (qty == 0) {
    return 0;
  }
  if (!validate_ean(ean)) {
    printf("invalid ean\n");
    return 0;
  }
  *b_idx = get_basket_idx(sys, ean);
  int current = (*b_idx != -1) ? sys->basket[*b_idx].amount : 0;
  if (qty < 0 && current + qty < 0) {
    printf("invalid quantity\n");
    return 0;
  }
  *c_idx = find_product_idx(sys, ean);
  if (*c_idx == -1) {
    printf("%s: no such product\n", ean);
    return 0;
  }
  if (qty > 0 && qty > sys->catalog[*c_idx].stock) {
    printf("no stock\n");
    return 0;
  }
  return 1;
}

void process_basket_add(SystemState *sys, Iva table[], const char *ean,
                        int qty) {
  int b_idx, c_idx;
  if (!validate_basket_add(sys, ean, qty, &b_idx, &c_idx)) {
    return;
  }

  int current = (b_idx != -1) ? sys->basket[b_idx].amount : 0;
  int new_amount = current + qty;

  sys->catalog[c_idx].stock -= qty;
  sys->catalog[c_idx].sold += qty;

  if (b_idx != -1) {
    sys->basket[b_idx].amount = new_amount;
  } else {
    insert_new_basket_item(sys, ean, new_amount);
  }

  print_basket_item(sys, table, c_idx, new_amount);
}