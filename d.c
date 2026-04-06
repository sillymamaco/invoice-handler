/*
 * Command 'd' implementation (delete invoice / reduce stock).
 * @file d.c
 * @author IST1117890 (Irina Cojocari)
 */

#include "d.h"
#include "a.h"
#include "l.h"
#include "p.h"

/*
 * Delete the invoice with the given ID.
 * @param sys System state.
 * @param inv_id Invoice ID to delete.
 */
static void cmd_d_delete_inv(SystemState *sys, int inv_id) {
  for (int ci = 0; ci < sys->client_count; ci++) {
    ClientRecord *cr = &sys->clients[ci];
    for (int ii = 0; ii < cr->invoice_count; ii++) {
      if (cr->invoices[ii].id != inv_id) {
        continue;
      }

      Invoice *inv = &cr->invoices[ii];
      printf("%.2f %d %s\n", inv->total_cents / 100.0, inv->nif, cr->name);

      sys->global_items -= inv->num_items;
      sys->global_sales_cents -= inv->total_cents;
      if (sys->global_sales_cents < 0) {
        sys->global_sales_cents = 0;
      }

      for (int k = ii; k < cr->invoice_count - 1; k++) {
        cr->invoices[k] = cr->invoices[k + 1];
      }
      cr->invoice_count--;
      return;
    }
  }
  printf("%d: no such invoice\n", inv_id);
}

/*
 * Reduce product stock by qty and print status.
 * @param sys System state.
 * @param cat_idx Catalog index.
 * @param qty Units to remove.
 */
static void remove_stock_and_print(SystemState *sys, int cat_idx, int qty) {
  sys->catalog[cat_idx].stock -= qty;
  if (sys->catalog[cat_idx].stock == 0) {
    printf("0 %s\n", sys->catalog[cat_idx].desc);
    free_safe(sys->catalog[cat_idx].desc,
              strlen(sys->catalog[cat_idx].desc) + 1, sys);
    sys->catalog[cat_idx].desc = NULL;
    for (int j = cat_idx; j < sys->catalog_count - 1; j++) {
      sys->catalog[j] = sys->catalog[j + 1];
    }
    sys->catalog_count--;
  } else {
    printf("%d %s\n", sys->catalog[cat_idx].stock, sys->catalog[cat_idx].desc);
  }
}

/*
 * Validates if stock can be reduced for a specific item.
 * @param sys System state.
 * @param ean EAN string.
 * @param qty Quantity to remove.
 * @param cat_idx Output pointer for catalog index.
 * @return Non-zero if valid.
 */
static int validate_stock_reduction(SystemState *sys, const char *ean, int qty,
                                    int *cat_idx) {
  if (!validate_ean(ean)) {
    printf("invalid ean\n");
    return 0;
  }
  *cat_idx = find_product_idx(sys, ean);
  if (*cat_idx == -1) {
    printf("%s: no such product\n", ean);
    return 0;
  }
  int b_idx = get_basket_idx(sys, ean);
  if (b_idx != -1 && sys->basket[b_idx].amount > 0) {
    printf("product in use\n");
    return 0;
  }
  if (qty <= 0 || qty > sys->catalog[*cat_idx].stock) {
    printf("invalid quantity\n");
    return 0;
  }
  return 1;
}

/*
 * Reduce product stock by qty.
 * @param sys System state.
 * @param ean Product EAN.
 * @param qty Units to remove.
 */
static void cmd_d_reduce_stock(SystemState *sys, const char *ean, int qty) {
  int cat_idx;
  if (validate_stock_reduction(sys, ean, qty, &cat_idx)) {
    remove_stock_and_print(sys, cat_idx, qty);
  }
}

/*
 * Delete an invoice or reduce product stock.
 * @param sys System state.
 */
void cmd_d(SystemState *sys) {
  char buf[MAX_INSTRC_LENGTH] = {0};
  if (!read_line_to_buffer(buf, MAX_INSTRC_LENGTH) && buf[0] == '\0')
    return;

  char arg1[BUFFER_LIMIT], arg2[BUFFER_LIMIT];
  int n = sscanf(buf, "%1023s %1023s", arg1, arg2);
  if (n == 1) {
    cmd_d_delete_inv(sys, atoi(arg1));
  } else if (n == 2) {
    if (strlen(arg1) > 13)
      printf("invalid ean\n");
    else
      cmd_d_reduce_stock(sys, arg1, atoi(arg2));
  }
}
