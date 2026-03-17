/**
 * @file commands.c
 * @brief Implementation of all system commands and basket logic.
 */

#include "commands.h"
#include "utils.h"
#include "memory.h"

static void cancel_basket(SystemState *sys) {
  for (int i = 0; i < sys->basket_count; i++) {
    int cat_idx = find_product_idx(sys, sys->basket[i].ean);
    if (cat_idx != -1) {
      sys->catalog[cat_idx].stock += sys->basket[i].amount;
      sys->catalog[cat_idx].sold -= sys->basket[i].amount;
    }
  }
  sys->basket_count = 0;
}

static void swap_basket(BasketItem *a, BasketItem *b) {
  BasketItem temp = *a; *a = *b; *b = temp;
}

static int partition_basket(BasketItem *arr, int low, int high) {
  char *pivot = arr[high].ean;
  int i = (low - 1);
  for (int j = low; j < high; j++) {
    if (strcmp(arr[j].ean, pivot) < 0) {
      i++; swap_basket(&arr[i], &arr[j]);
    }
  }
  swap_basket(&arr[i + 1], &arr[high]);
  return (i + 1);
}

static void quick_sort_basket(BasketItem *arr, int low, int high) {
  if (low < high) {
    int pi = partition_basket(arr, low, high);
    quick_sort_basket(arr, low, pi - 1);
    quick_sort_basket(arr, pi + 1, high);
  }
}

static void print_sorted_basket(SystemState *sys, Iva table[], int iva_count) {
  if (sys->basket_count > 1)
    quick_sort_basket(sys->basket, 0, sys->basket_count - 1);
  for (int i = 0; i < sys->basket_count; i++) {
    int cat_idx = find_product_idx(sys, sys->basket[i].ean);
    if (cat_idx != -1)
      print_basket_item(sys, table, iva_count, cat_idx, sys->basket[i].amount);
  }
}

static void process_basket_add(SystemState *sys, Iva table[], int iva_count, const char *ean, int qty) {
  int cat_idx = find_product_idx(sys, ean), basket_idx = -1;
  for (int j = 0; j < sys->basket_count; j++) {
    if (strcmp(sys->basket[j].ean, ean) == 0) { basket_idx = j; break; }
  }

  if (cat_idx == -1) { printf("%s: no such product\n", ean); return; }
  
  if (qty > sys->catalog[cat_idx].stock ||
      (qty < 0 && (basket_idx == -1 || sys->basket[basket_idx].amount + qty < 0))) {
    printf("no stock\n"); return;
  }

  sys->catalog[cat_idx].stock -= qty;
  sys->catalog[cat_idx].sold += qty;
  if (basket_idx != -1) {
    sys->basket[basket_idx].amount += qty;
  } else {
    if (sys->basket_count == sys->basket_capacity) {
      int new_cap = sys->basket_capacity ? sys->basket_capacity * 2 : 10;
      sys->basket = safe_realloc(sys->basket, sys->basket_capacity * sizeof(BasketItem), new_cap * sizeof(BasketItem), sys);
      sys->basket_capacity = new_cap;
    }
    basket_idx = sys->basket_count++;
    strcpy(sys->basket[basket_idx].ean, ean);
    sys->basket[basket_idx].amount = qty;
  }
  print_basket_item(sys, table, iva_count, cat_idx, sys->basket[basket_idx].amount);
}

static void finalize_invoice(SystemState *sys, Iva table[], int iva_count, int nif, const char *name) {
  double total = 0;
  int items = 0;
  for (int i = 0; i < sys->basket_count; i++) {
    int cat_idx = find_product_idx(sys, sys->basket[i].ean);
    if (cat_idx != -1 && sys->basket[i].amount > 0) {
      items += sys->basket[i].amount;
      int iva = get_iva_rate(table, iva_count, sys->catalog[cat_idx].iva_class);
      double sub = (sys->catalog[cat_idx].price * sys->basket[i].amount) * (1.0 + (iva / 100.0));
      total += round_money(sub);
    }
  }
  
  if (sys->history_count == sys->history_capacity) {
    int new_cap = sys->history_capacity ? sys->history_capacity * 2 : 10;
    sys->history = safe_realloc(sys->history, sys->history_capacity * sizeof(Invoice), new_cap * sizeof(Invoice), sys);
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
    } else { break; }
  }
  sys->history[i + 1] = new_inv;
  sys->history_count++;

  sys->global_items += items;
  sys->global_sales += total; 
  printf("%d %.2f %d\n", items, total, new_inv.id);
  sys->basket_count = 0;
}

static void cmd_d_delete_inv(SystemState *sys, int inv_id) {
  int h_idx = -1;
  for (int j = 0; j < sys->history_count; j++)
    if (sys->history[j].id == inv_id) { h_idx = j; break; }
  
  if (h_idx == -1) { printf("%d: no such invoice\n", inv_id); return; }

  printf("%.2f %d %s\n", sys->history[h_idx].total, sys->history[h_idx].nif, sys->history[h_idx].client_name);
  sys->global_items -= sys->history[h_idx].num_items;
  sys->global_sales -= sys->history[h_idx].total;
  free_safe(sys->history[h_idx].client_name, strlen(sys->history[h_idx].client_name) + 1, sys);

  for (int j = h_idx; j < sys->history_count - 1; j++)
    sys->history[j] = sys->history[j + 1];
  sys->history_count--;
}

static void cmd_d_reduce_stock(SystemState *sys, const char *ean, int qty) {
  int cat_idx = find_product_idx(sys, ean);
  if (cat_idx == -1) { printf("%s: no such product\n", ean); return; }
  if (qty <= 0 || qty > sys->catalog[cat_idx].stock) { printf("invalid quantity\n"); return; }

  int in_basket = 0;
  for (int j = 0; j < sys->basket_count; j++) {
    if (strcmp(sys->basket[j].ean, ean) == 0) { in_basket = sys->basket[j].amount; break; }
  }

  if (sys->catalog[cat_idx].stock - qty < in_basket) { printf("product in use\n"); return; }

  sys->catalog[cat_idx].stock -= qty;
  if (sys->catalog[cat_idx].stock == 0) {
    printf("0 %s\n", sys->catalog[cat_idx].desc);
    free_safe(sys->catalog[cat_idx].desc, strlen(sys->catalog[cat_idx].desc) + 1, sys);
    for (int j = cat_idx; j < sys->catalog_count - 1; j++)
      sys->catalog[j] = sys->catalog[j + 1];
    sys->catalog_count--;
  } else {
    printf("%d %s\n", sys->catalog[cat_idx].stock, sys->catalog[cat_idx].desc);
  }
}

void cmd_p(SystemState *sys, Iva table[], int iva_count) {
  char ean[14], iva_c;
  double price;
  int stock;
  if (scanf("%13s %c %lf %d", ean, &iva_c, &price, &stock) != 4) return;
  char *desc = read_token_safe(sys);

  int iva_ok = 0;
  for (int i = 0; i < iva_count; i++)
    if (table[i].letter == iva_c) iva_ok = 1;
  
  if (!validate_p_input(ean, iva_ok, price, stock, desc)) {
    if (desc) free_safe(desc, strlen(desc) + 1, sys);
    return;
  }

  int idx = find_product_idx(sys, ean);
  if (idx != -1) {
    for (int i = 0; i < sys->basket_count; i++) {
      if (strcmp(sys->basket[i].ean, ean) == 0 && sys->catalog[idx].price != price) {
        printf("product in use\n");
        free_safe(desc, strlen(desc) + 1, sys);
        return;
      }
    }
    sys->catalog[idx].iva_class = iva_c;
    sys->catalog[idx].price = price;
    sys->catalog[idx].stock += stock;
    free_safe(sys->catalog[idx].desc, strlen(sys->catalog[idx].desc) + 1, sys);
    sys->catalog[idx].desc = desc;
  } else {
    if (sys->catalog_count == sys->catalog_capacity) {
      int new_cap = sys->catalog_capacity ? sys->catalog_capacity * 2 : 10;
      sys->catalog = safe_realloc(sys->catalog, sys->catalog_capacity * sizeof(Product), new_cap * sizeof(Product), sys);
      sys->catalog_capacity = new_cap;
    }

    int i = sys->catalog_count - 1;
    while (i >= 0 && strcmp(sys->catalog[i].ean, ean) > 0) {
      sys->catalog[i + 1] = sys->catalog[i];
      i--;
    }
    idx = i + 1;

    strcpy(sys->catalog[idx].ean, ean);
    sys->catalog[idx].iva_class = iva_c;
    sys->catalog[idx].price = price;
    sys->catalog[idx].stock = stock;
    sys->catalog[idx].desc = desc;
    sys->catalog[idx].sold = 0;
    sys->catalog[idx].insert_order = sys->catalog_count;
    sys->catalog_count++;
  }
  printf("%d\n", sys->catalog[idx].stock);
}

void cmd_l(SystemState *sys) {
  int c;
  while ((c = getchar()) == ' ' || c == '\t' || c == '\r');
  
  Product **ordered = NULL;
  if (sys->catalog_count > 0) {
      ordered = safemalloc(sys->catalog_count * sizeof(Product*), sys);
      for (int k = 0; k < sys->catalog_count; k++) ordered[k] = &sys->catalog[k];
      qsort(ordered, sys->catalog_count, sizeof(Product*), cmp_insert_order);
  }

  if (c == '\n' || c == EOF) {
    int found = 0;
    for (int i = 0; i < sys->catalog_count; i++) {
      if (ordered[i]->stock > 0) { print_product(ordered[i]); found = 1; }
    }
    if (!found) printf("*: no such product\n");
    if (ordered) free_safe(ordered, sys->catalog_count * sizeof(Product*), sys);
    return;
  }

  char buf[MAX_INSTRC_LENGTH] = {0};
  int i = 0;
  buf[i++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF)
    if (i < MAX_INSTRC_LENGTH - 1 && c != '\r') buf[i++] = (char)c;
  buf[i] = '\0';

  char *token = strtok(buf, " \t\r\n");
  while (token) {
    int found_any = 0;
    int has_wildcard = (strchr(token, '*') != NULL || strchr(token, '?') != NULL);

    if (!has_wildcard) {
      int idx = find_product_idx(sys, token);
      if (idx != -1 && sys->catalog[idx].stock > 0) {
        print_product(&sys->catalog[idx]); found_any = 1;
      }
    } else {
      for (int j = 0; j < sys->catalog_count; j++) {
        if (match(token, ordered[j]->ean) && ordered[j]->stock > 0) {
          print_product(ordered[j]); found_any = 1;
        }
      }
    }
    if (!found_any) printf("%s: no such product\n", token);
    token = strtok(NULL, " \t\r\n");
  }
  if (ordered) free_safe(ordered, sys->catalog_count * sizeof(Product*), sys);
}

void cmd_a(SystemState *sys, Iva table[], int iva_count) {
  int c;
  while ((c = getchar()) == ' ' || c == '\t' || c == '\r');
  if (c == '\n' || c == EOF) {
    print_sorted_basket(sys, table, iva_count);
    return;
  }

  char buf[BUFFER_LIMIT] = {0};
  int i = 0;
  buf[i++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF)
    if (i < BUFFER_LIMIT - 1 && c != '\r') buf[i++] = (char)c;
  buf[i] = '\0';

  char s1[BUFFER_LIMIT], s2[BUFFER_LIMIT], ean[14];
  int qty = 1;
  if (sscanf(buf, "%1023s %13s", s1, s2) == 2) {
    qty = atoi(s1);
    strcpy(ean, s2);
  } else {
    strcpy(ean, s1);
  }

  if (qty != 0) process_basket_add(sys, table, iva_count, ean, qty);
}

void cmd_f(SystemState *sys, Iva table[], int iva_count) {
  char *line = read_token_safe(sys);
  int nif = 999999999;
  char *name = "Cliente final";

  if (line) parse_invoice_client(line, &nif, &name);
  if (nif != 999999999 && (nif < 100000000 || nif > 999999999)) {
    printf("%d: no such nif\n", nif);
    if (line) free_safe(line, strlen(line) + 1, sys);
    return;
  }
  if (strcmp(name, "error") == 0) {
    cancel_basket(sys);
    if (line) free_safe(line, strlen(line) + 1, sys);
    return;
  }
  if (!is_valid_name_start(name) && strcmp(name, "Cliente final") != 0) {
    printf("invalid name\n");
    if (line) free_safe(line, strlen(line) + 1, sys);
    return;
  }

  finalize_invoice(sys, table, iva_count, nif, name);
  if (line) free_safe(line, strlen(line) + 1, sys);
}

void cmd_r(SystemState *sys, Iva table[], int iva_count) {
  int c;
  while ((c = getchar()) == ' ' || c == '\t' || c == '\r');
  if (c == '\n' || c == EOF) {
    printf("%d %d %.2f\n", sys->global_items, sys->next_invoice_id - 1, sys->global_sales);
    Iva sorted_table[26];
    memcpy(sorted_table, table, iva_count * sizeof(Iva));
    for (int i = 1; i < iva_count; i++) {
      Iva key = sorted_table[i];
      int j = i - 1;
      while (j >= 0 && sorted_table[j].letter > key.letter) {
        sorted_table[j + 1] = sorted_table[j]; j--;
      }
      sorted_table[j + 1] = key;
    }
    for (int i = 0; i < iva_count; i++)
      printf("%c %d%%\n", sorted_table[i].letter, sorted_table[i].value);
    return;
  }

  char ean[14];
  ean[0] = (char)c;
  scanf("%12s", ean + 1);
  int cat_idx = find_product_idx(sys, ean);
  if (cat_idx == -1) printf("%s: no such product\n", ean);
  else printf("%d %d %s\n", sys->catalog[cat_idx].stock, sys->catalog[cat_idx].sold, sys->catalog[cat_idx].desc);
}

void cmd_c(SystemState *sys) {
  char *line = read_token_safe(sys);
  char name[BUFFER_LIMIT] = "";
  if (line) {
    char *ptr = line;
    while (*ptr && isspace(*ptr)) ptr++;
    if (*ptr) {
      if (*ptr == '"') {
        ptr++; char *end = strchr(ptr, '"');
        if (end) *end = '\0';
        strcpy(name, ptr);
      } else {
        strcpy(name, ptr);
        char *end = name;
        while (*end && !isspace((unsigned char)*end)) end++;
        *end = '\0';
      }
    }
  }

  if (strlen(name) > 0 && !is_valid_name_start(name)) {
    printf("invalid name\n");
    if (line) free_safe(line, strlen(line) + 1, sys);
    return;
  }

  if (strlen(name) == 0) {
    for (int i = 0; i < sys->history_count; i++)
      printf("%d %.2f %s\n", sys->history[i].id, sys->history[i].total, sys->history[i].client_name);
  } else {
    int found = 0;
    if (sys->history_count == 0) {
      printf("%s: no such client\n", name);
      if (line) free_safe(line, strlen(line) + 1, sys);
      return;
    }
    
    int *match_idx = safemalloc(sys->history_count * sizeof(int), sys);
    int match_count = 0;
    
    for (int i = 0; i < sys->history_count; i++) {
      if (strcmp(sys->history[i].client_name, name) == 0) match_idx[match_count++] = i;
    }
    for (int i = 1; i < match_count; i++) {
      int key = match_idx[i]; int j = i - 1;
      while (j >= 0 && sys->history[match_idx[j]].id > sys->history[key].id) {
        match_idx[j + 1] = match_idx[j]; j--;
      }
      match_idx[j + 1] = key;
    }
    for (int i = 0; i < match_count; i++) {
      int mi = match_idx[i];
      printf("%d %.2f %s\n", sys->history[mi].id, sys->history[mi].total, sys->history[mi].client_name);
      found = 1;
    }
    free_safe(match_idx, sys->history_count * sizeof(int), sys);
    if (!found) printf("%s: no such client\n", name);
  }
  if (line) free_safe(line, strlen(line) + 1, sys);
}

void cmd_d(SystemState *sys) {
  int c;
  while ((c = getchar()) == ' ' || c == '\t' || c == '\r');
  if (c == '\n' || c == EOF) return;

  char buf[MAX_INSTRC_LENGTH] = {0};
  int i = 0;
  buf[i++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF)
    if (i < MAX_INSTRC_LENGTH - 1 && c != '\r') buf[i++] = (char)c;
  buf[i] = '\0';

  char arg1[BUFFER_LIMIT], arg2[BUFFER_LIMIT];
  int num_args = sscanf(buf, "%1023s %1023s", arg1, arg2);

  if (num_args == 1) cmd_d_delete_inv(sys, atoi(arg1));
  else if (num_args == 2) cmd_d_reduce_stock(sys, arg1, atoi(arg2));
}