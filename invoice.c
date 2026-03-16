#include "invoice.h"
#include "catalog.h"
#include "memory.h"
#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

void parse_invoice_client(char *line, int *nif, char **name) {
  char *ptr = line;
  while (*ptr && isspace(*ptr))
    ptr++;
  if (isdigit(*ptr)) {
    sscanf(ptr, "%d", nif);
    while (*ptr && !isspace(*ptr))
      ptr++;
  }
  while (*ptr && isspace(*ptr))
    ptr++;
  if (*ptr) {
    if (*ptr == '"') {
      ptr++;
      char *end = strchr(ptr, '"');
      if (end) {
        *end = '\0';
        *name = ptr;
      } else {
        *name = "1_invalid";
      }
    } else {
      *name = ptr;
      char *end = *name;
      while (*end && !isspace((unsigned char)*end))
        end++;
      *end = '\0';
    }
  }
}

void finalize_invoice(SystemState *sys, Iva table[], int iva_count, int nif,
                      const char *name) {
  double total = 0;
  int items = 0;
  for (int i = 0; i < sys->basket_count; i++) {
    int cat_idx = find_product_idx(sys, sys->basket[i].ean);
    if (cat_idx != -1 && sys->basket[i].amount > 0) {
      items += sys->basket[i].amount;
      int iva = get_iva_rate(table, iva_count, sys->catalog[cat_idx].iva_class);
      double sub = (sys->catalog[cat_idx].price * sys->basket[i].amount) *
                   (1.0 + (iva / PCT_DIV));
      total += round_money(sub);
    }
  }
  if (sys->history_count == sys->history_capacity) {
    int new_cap = sys->history_capacity ? sys->history_capacity * 2 : BASE_CAP;
    sys->history =
        safe_realloc(sys->history, sys->history_capacity * sizeof(Invoice),
                     new_cap * sizeof(Invoice), sys);
    sys->history_capacity = new_cap;
  }
  Invoice new_inv;
  new_inv.nif = nif;
  new_inv.total = total;
  new_inv.id = sys->next_invoice_id++;
  new_inv.num_items = items;
  new_inv.client_name = safemalloc(strlen(name) + 1, sys);
  strcpy(new_inv.client_name, name);

  int i = sys->history_count - 1;
  while (i >= 0) {
    int cmp = strcmp(sys->history[i].client_name, new_inv.client_name);
    if (cmp > 0 || (cmp == 0 && sys->history[i].id > new_inv.id)) {
      sys->history[i + 1] = sys->history[i];
      i--;
    } else {
      break;
    }
  }
  sys->history[i + 1] = new_inv;
  sys->history_count++;

  sys->global_items += items;
  sys->global_sales += total;
  printf("%d %.2f %d\n", items, total, new_inv.id);
  sys->basket_count = 0;
}

void cmd_d_delete_inv(SystemState *sys, int inv_id) {
  int h_idx = -1;
  for (int j = 0; j < sys->history_count; j++) {
    if (sys->history[j].id == inv_id) {
      h_idx = j;
      break;
    }
  }
  if (h_idx == -1) {
    printf("%d: no such invoice\n", inv_id);
    return;
  }
  printf("%.2f %d %s\n", sys->history[h_idx].total, sys->history[h_idx].nif,
         sys->history[h_idx].client_name);
  sys->global_items -= sys->history[h_idx].num_items;
  sys->global_sales -= sys->history[h_idx].total;

  size_t len = strlen(sys->history[h_idx].client_name) + 1;
  free_safe(sys->history[h_idx].client_name, len, sys);

  for (int j = h_idx; j < sys->history_count - 1; j++) {
    sys->history[j] = sys->history[j + 1];
  }
  sys->history_count--;
}