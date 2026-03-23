/**
 * @file catalog.c
 * @author IST1117890 (Irina Cojocari)
 * @brief Product catalog management.
 */

#include "commands.h"
#include "memory.h"
#include "shared.h"
#include "utils.h"

void drain_line(void) {
  int c;
  while ((c = getchar()) != '\n' && c != EOF)
    ;
}

int read_p_line(char *buf, int bufsz) {
  int c;
  while ((c = getchar()) == ' ' || c == '\t' || c == '\r')
    ;
  if (c == '\n' || c == EOF)
    return 0;

  int li = 0;
  buf[li++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF) {
    if (c == '\r')
      continue;
    if (li < bufsz - 1)
      buf[li++] = (char)c;
  }
  buf[li] = '\0';
  return 1;
}

char *extract_desc(const char *line, SystemState *sys) {
  const char *dp = line;
  int skipped = 0;
  while (*dp && skipped < 4) {
    while (*dp && isspace((unsigned char)*dp))
      dp++;
    while (*dp && !isspace((unsigned char)*dp))
      dp++;
    skipped++;
  }
  while (*dp && isspace((unsigned char)*dp))
    dp++;
  if (!*dp)
    return NULL;

  int dlen = (int)strlen(dp);
  while (dlen > 0 && isspace((unsigned char)dp[dlen - 1]))
    dlen--;
  if (dlen <= 0)
    return NULL;

  char *desc = safemalloc(dlen + 1, sys);
  memcpy(desc, dp, dlen);
  desc[dlen] = '\0';
  return desc;
}

void catalog_insert(SystemState *sys, const char *ean, char iva_c, double price,
                    int stock, char *desc) {
  if (sys->catalog_count == sys->catalog_capacity) {
    int nc = sys->catalog_capacity ? sys->catalog_capacity * 2 : 10;
    sys->catalog =
        safe_realloc(sys->catalog, sys->catalog_capacity * sizeof(Product),
                     nc * sizeof(Product), sys);
    sys->catalog_capacity = nc;
  }
  int i = sys->catalog_count - 1;
  while (i >= 0 && strcmp(sys->catalog[i].ean, ean) > 0) {
    sys->catalog[i + 1] = sys->catalog[i];
    i--;
  }
  int idx = i + 1;
  strcpy(sys->catalog[idx].ean, ean);
  sys->catalog[idx].iva_class = iva_c;
  sys->catalog[idx].price = price;
  sys->catalog[idx].stock = stock;
  sys->catalog[idx].desc = desc;
  sys->catalog[idx].sold = 0;
  sys->catalog[idx].insert_order = sys->next_product_order++;
  sys->catalog_count++;
}

int catalog_update(SystemState *sys, int idx, const char *ean, char iva_c,
                   double price, int stock, char *desc) {
  for (int i = 0; i < sys->basket_count; i++) {
    if (strcmp(sys->basket[i].ean, ean) == 0 && sys->basket[i].amount > 0 &&
        sys->catalog[idx].price != price) {
      printf("product in use\n");
      return 0;
    }
  }
  sys->catalog[idx].iva_class = iva_c;
  sys->catalog[idx].price = price;
  sys->catalog[idx].stock += stock;
  free_safe(sys->catalog[idx].desc, strlen(sys->catalog[idx].desc) + 1, sys);
  sys->catalog[idx].desc = desc;
  return 1;
}

static double parse_price_field(const char *price_str) {
  char *pe = NULL;
  double price = strtod(price_str, &pe);
  if (pe == price_str || *pe != '\0')
    return -1.0;
  for (const char *pc = price_str; *pc; pc++) {
    if (*pc == 'e' || *pc == 'E' || *pc == '+' || *pc == '-')
      return -1.0;
  }
  return price;
}

static int parse_stock_field(const char *stock_str) {
  char *se = NULL;
  long sl = strtol(stock_str, &se, 10);
  if (se == stock_str || *se != '\0' || stock_str[0] == '+')
    return -1;
  return (int)sl;
}

int parse_p_fields(const char *linebuf, char ean[14], char *iva_c,
                   double *price, int *stock) {
  char ean_raw[16] = {0}, iva_str[16] = {0}, price_str[32] = {0},
       stock_str[32] = {0};

  if (sscanf(linebuf, "%15s %15s %31s %31s", ean_raw, iva_str, price_str,
             stock_str) < 4)
    return 0;

  size_t rlen = strlen(ean_raw);
  if (rlen > 13)
    return 0;
  memcpy(ean, ean_raw, rlen);
  ean[rlen] = '\0';

  *iva_c = iva_str[0];
  if (iva_str[1] != '\0')
    *iva_c = '\0';

  *price = parse_price_field(price_str);
  *stock = parse_stock_field(stock_str);

  return 1;
}

static void swap_ordered(Product **a, Product **b) {
  Product *t = *a;
  *a = *b;
  *b = t;
}

static int partition_ordered(Product **arr, int lo, int hi) {
  int pivot = arr[hi]->insert_order, i = lo - 1;
  for (int j = lo; j < hi; j++) {
    if (arr[j]->insert_order < pivot) {
      i++;
      swap_ordered(&arr[i], &arr[j]);
    }
  }
  swap_ordered(&arr[i + 1], &arr[hi]);
  return i + 1;
}

static void quick_sort_ordered(Product **arr, int lo, int hi) {
  if (lo < hi) {
    int pi = partition_ordered(arr, lo, hi);
    quick_sort_ordered(arr, lo, pi - 1);
    quick_sort_ordered(arr, pi + 1, hi);
  }
}

Product **build_ordered(SystemState *sys) {
  if (sys->catalog_count == 0)
    return NULL;
  Product **ordered = safemalloc(sys->catalog_count * sizeof(Product *), sys);
  for (int k = 0; k < sys->catalog_count; k++)
    ordered[k] = &sys->catalog[k];
  if (sys->catalog_count > 1)
    quick_sort_ordered(ordered, 0, sys->catalog_count - 1);
  return ordered;
}

int print_l_token(SystemState *sys, Product **ordered, const char *token) {
  int found = 0;
  if (strchr(token, '*') || strchr(token, '?')) {
    for (int j = 0; j < sys->catalog_count; j++) {
      if (match(token, ordered[j]->ean) && ordered[j]->stock > 0) {
        print_product(ordered[j]);
        found = 1;
      }
    }
  } else {
    int idx = find_product_idx(sys, token);
    if (idx != -1 && sys->catalog[idx].stock > 0) {
      print_product(&sys->catalog[idx]);
      found = 1;
    }
  }
  return found;
}

void cmd_l_all(SystemState *sys, Product **ordered) {
  int found = 0;
  for (int i = 0; i < sys->catalog_count; i++) {
    if (ordered[i]->stock > 0) {
      print_product(ordered[i]);
      found = 1;
    }
  }
  if (!found)
    printf("*: no such product\n");
}

void cmd_l_tokens(SystemState *sys, Product **ordered, char *buf) {
  char *token = strtok(buf, " \t\r\n");
  while (token) {
    if (!print_l_token(sys, ordered, token))
      printf("%s: no such product\n", token);
    token = strtok(NULL, " \t\r\n");
  }
}

void cmd_p(SystemState *sys, Iva table[]) {
  char linebuf[MAX_INSTRC_LENGTH] = {0};
  if (!read_p_line(linebuf, MAX_INSTRC_LENGTH))
    return;

  char ean[14] = {0}, iva_c = '\0';
  double price = 0.0;
  int stock = 0;
  if (!parse_p_fields(linebuf, ean, &iva_c, &price, &stock))
    return;

  int iva_ok = (iva_c != '\0') && iva_is_present(table, iva_c);
  char *desc = extract_desc(linebuf, sys);

  if (!validate_p_input(ean, iva_ok, price, stock, desc)) {
    if (desc)
      free_safe(desc, strlen(desc) + 1, sys);
    return;
  }

  int idx = find_product_idx(sys, ean);
  if (idx != -1) {
    if (!catalog_update(sys, idx, ean, iva_c, price, stock, desc)) {
      free_safe(desc, strlen(desc) + 1, sys);
      return;
    }
  } else {
    if (sys->catalog_count >= 10000) {
      printf("invalid product\n");
      free_safe(desc, strlen(desc) + 1, sys);
      return;
    }
    catalog_insert(sys, ean, iva_c, price, stock, desc);
    idx = find_product_idx(sys, ean);
  }
  printf("%d\n", sys->catalog[idx].stock);
}

void cmd_l(SystemState *sys) {
  char buf[MAX_INSTRC_LENGTH] = {0};
  Product **ordered = build_ordered(sys);

  if (!read_line_to_buffer(buf, MAX_INSTRC_LENGTH) && buf[0] == '\0') {
    cmd_l_all(sys, ordered);
  } else {
    cmd_l_tokens(sys, ordered, buf);
  }

  if (ordered)
    free_safe(ordered, sys->catalog_count * sizeof(Product *), sys);
}

/**
 * @brief Handles bare r command to print metrics.
 * @param sys System state.
 * @param table IVA rate table.
 */
static void cmd_r_global_summary(SystemState *sys, Iva table[]) {
  printf("%d %d %.2f\n", sys->global_items, sys->next_invoice_id - 1,
         sys->global_sales_cents / 100.0);
  for (int i = 0; i < IVA_TABLE_SIZE; i++) {
    if (table[i].present)
      printf("%c %d%%\n", (char)('A' + i), table[i].value);
  }
}

void cmd_r(SystemState *sys, Iva table[]) {
  char buf[MAX_INSTRC_LENGTH] = {0};

  if (!read_line_to_buffer(buf, MAX_INSTRC_LENGTH) && buf[0] == '\0') {
    cmd_r_global_summary(sys, table);
    return;
  }

  char ean[14] = {0};
  int ei = 0;
  for (int i = 0; buf[i] && !isspace((unsigned char)buf[i]); i++) {
    if (ei < 13)
      ean[ei++] = buf[i];
  }
  ean[ei] = '\0';

  if (strlen(ean) == 0 || !validate_ean(ean)) {
    printf("invalid ean\n");
    return;
  }

  int cat_idx = find_product_idx(sys, ean);
  if (cat_idx == -1)
    printf("%s: no such product\n", ean);
  else
    printf("%d %d %s\n", sys->catalog[cat_idx].stock,
           sys->catalog[cat_idx].sold, sys->catalog[cat_idx].desc);
}