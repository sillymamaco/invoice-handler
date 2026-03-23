/**
 * @file invoice.c
 * @author IST1117890 (Irina Cojocari)
 * @brief Client record management and invoice operations.
 */

#include "commands.h"
#include "common.h"
#include "memory.h"
#include "shared.h"
#include "utils.h"

int get_or_create_client(SystemState *sys, const char *name, int nif) {
  int idx = find_client_idx(sys, name);
  if (idx != -1)
    return idx;

  if (sys->client_count == sys->client_capacity) {
    int nc = sys->client_capacity ? sys->client_capacity * 2 : 8;
    sys->clients =
        safe_realloc(sys->clients, sys->client_capacity * sizeof(ClientRecord),
                     nc * sizeof(ClientRecord), sys);
    sys->client_capacity = nc;
  }

  int i = sys->client_count - 1;
  while (i >= 0 && cmp_names(sys->clients[i].name, name) > 0) {
    sys->clients[i + 1] = sys->clients[i];
    i--;
  }
  idx = i + 1;

  ClientRecord *cr = &sys->clients[idx];
  cr->name = safemalloc(strlen(name) + 1, sys);
  strcpy(cr->name, name);
  cr->nif = nif;
  cr->invoices = NULL;
  cr->invoice_count = 0;
  cr->invoice_cap = 0;
  sys->client_count++;
  return idx;
}

/**
 * @brief Accumulate total basket value and item count.
 * @param sys System state.
 * @param table IVA table.
 * @param total_cents Pointer to store total price.
 * @param items Pointer to store total item count.
 */
static void calculate_basket_totals(SystemState *sys, Iva table[],
                                    long long *total_cents, int *items) {
  for (int i = 0; i < sys->basket_count; i++) {
    int cat_idx = find_product_idx(sys, sys->basket[i].ean);
    if (cat_idx != -1 && sys->basket[i].amount > 0) {
      *items += sys->basket[i].amount;
      int iva = get_iva_rate(table, sys->catalog[cat_idx].iva_class);
      long long price_cents =
          (long long)(sys->catalog[cat_idx].price * 100.0 + 0.5);
      long long numerator = price_cents * sys->basket[i].amount * (100 + iva);
      *total_cents += (numerator + 50) / 100;
    }
  }
}

void finalize_invoice(SystemState *sys, Iva table[], int nif,
                      const char *name) {
  long long total_cents = 0;
  int items = 0;

  calculate_basket_totals(sys, table, &total_cents, &items);

  int ci = get_or_create_client(sys, name, nif);
  ClientRecord *cr = &sys->clients[ci];

  if (cr->invoice_count == cr->invoice_cap) {
    int nc = cr->invoice_cap ? cr->invoice_cap * 2 : 4;
    cr->invoices = safe_realloc(cr->invoices, cr->invoice_cap * sizeof(Invoice),
                                nc * sizeof(Invoice), sys);
    cr->invoice_cap = nc;
  }

  Invoice *inv = &cr->invoices[cr->invoice_count++];
  inv->nif = nif;
  inv->total_cents = total_cents;
  inv->id = sys->next_invoice_id++;
  inv->num_items = items;

  sys->global_items += items;
  sys->global_sales_cents += total_cents;

  printf("%d %.2f %d\n", items, total_cents / 100.0, inv->id);
  sys->basket_count = 0;
}

void cmd_d_delete_inv(SystemState *sys, int inv_id) {
  for (int ci = 0; ci < sys->client_count; ci++) {
    ClientRecord *cr = &sys->clients[ci];
    for (int ii = 0; ii < cr->invoice_count; ii++) {
      if (cr->invoices[ii].id != inv_id)
        continue;

      Invoice *inv = &cr->invoices[ii];
      printf("%.2f %d %s\n", inv->total_cents / 100.0, inv->nif, cr->name);

      sys->global_items -= inv->num_items;
      sys->global_sales_cents -= inv->total_cents;
      if (sys->global_sales_cents < 0)
        sys->global_sales_cents = 0;

      for (int k = ii; k < cr->invoice_count - 1; k++)
        cr->invoices[k] = cr->invoices[k + 1];
      cr->invoice_count--;
      return;
    }
  }
  printf("%d: no such invoice\n", inv_id);
}

/**
 * @brief Check if an item is reserved in the basket.
 * @param sys System state.
 * @param ean EAN string.
 * @return Reserved quantity or 0.
 */
static int get_basket_reserved_qty(SystemState *sys, const char *ean) {
  for (int j = 0; j < sys->basket_count; j++) {
    if (strcmp(sys->basket[j].ean, ean) == 0)
      return sys->basket[j].amount;
  }
  return 0;
}

void cmd_d_reduce_stock(SystemState *sys, const char *ean, int qty) {
  if (!validate_ean(ean)) {
    printf("invalid ean\n");
    return;
  }
  int cat_idx = find_product_idx(sys, ean);
  if (cat_idx == -1) {
    printf("%s: no such product\n", ean);
    return;
  }

  if (get_basket_reserved_qty(sys, ean) > 0) {
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
    free_safe(sys->catalog[cat_idx].desc,
              strlen(sys->catalog[cat_idx].desc) + 1, sys);
    sys->catalog[cat_idx].desc = NULL;
    for (int j = cat_idx; j < sys->catalog_count - 1; j++)
      sys->catalog[j] = sys->catalog[j + 1];
    sys->catalog_count--;
  } else {
    printf("%d %s\n", sys->catalog[cat_idx].stock, sys->catalog[cat_idx].desc);
  }
}