/*
 * Command 'f' implementation (invoice finalization).
 * @file f.c
 * @author IST1117890 (Irina Cojocari)
 */

#include "f.h"
#include "c.h"
#include "p.h"
#include "r.h"

/*
 * Return all basket quantities to stock and reset basket_count to 0.
 * @param sys System state.
 */
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

/*
 * Return the index of a ClientRecord, inserting a new one if needed.
 * @param sys System state.
 * @param name Client name.
 * @param nif Client NIF.
 * @return Index of the client.
 */
int get_or_create_client(SystemState *sys, const char *name, int nif) {
  int idx = find_client_idx(sys, name);
  if (idx != -1) {
    return idx;
  }

  if (sys->client_count == sys->client_capacity) {
    int nc = sys->client_capacity ? sys->client_capacity * 2 : 8;
    sys->clients =
        safe_realloc(sys->clients, sys->client_capacity * sizeof(ClientRecord),
                     nc * sizeof(ClientRecord), sys);
    sys->client_capacity = nc;
  }

  int i = sys->client_count - 1;
  /* insert the client in the right place to avoid sorting later */
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

/*
 * Accumulate total basket value and item count.
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

/*
 * Close the basket and append an invoice to the client's queue.
 * @param sys System state.
 * @param table IVA rate table.
 * @param nif Client NIF.
 * @param name Client name.
 */
static void finalize_invoice(SystemState *sys, Iva table[], int nif,
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

/*
 * Handles nif and name distinction if only one argument is provided.
 * @param start Word pointer.
 * @param len Word length.
 * @param nif_raw Output NIF buffer.
 * @param name_buf Output name buffer.
 * @param has_name Output flag for name.
 */
static void handle_f_single_word(const char *start, size_t len, char *nif_raw,
                                 char *name_buf, int *has_name) {
  char w1[MAX_INSTRC_LENGTH];
  strncpy(w1, start, len);
  w1[len] = '\0';
  int all_digits = 1;
  for (size_t i = 0; i < len; i++) {
    if (!isdigit((unsigned char)w1[i])) {
      all_digits = 0;
      break;
    }
  }
  if (all_digits)
    strcpy(nif_raw, w1);
  else {
    strcpy(name_buf, w1);
    *has_name = 1;
  }
}

/*
 * Handles nifs and names when both arguments are provided.
 * @param start NIF word pointer.
 * @param len NIF length.
 * @param after_w1 Name start pointer.
 * @param nif_raw Output NIF buffer.
 * @param name_buf Output name buffer.
 * @param has_name Output flag for name.
 * @param valid_name Output flag for name validity.
 */
static void handle_f_multi_word(const char *start, size_t len,
                                const char *after_w1, char *nif_raw,
                                char *name_buf, int *has_name,
                                int *valid_name) {
  strncpy(nif_raw, start, len);
  nif_raw[len] = '\0';
  const char *ptr = after_w1;
  *has_name = 1;
  if (*ptr == '"') {
    if (!extract_quoted_string(&ptr, name_buf, MAX_INSTRC_LENGTH))
      *valid_name = 0;
  } else {
    const char *ns = ptr;
    while (*ptr && !isspace((unsigned char)*ptr))
      ptr++;
    size_t nlen = ptr - ns;
    strncpy(name_buf, ns, nlen);
    name_buf[nlen] = '\0';
  }
  while (*ptr && isspace((unsigned char)*ptr))
    ptr++;
  if (*ptr != '\0')
    *valid_name = 0;
}

/*
 * Handles arguments when no quotes are present at the start of the line.
 * @param ptr Pointer to the first non-space character.
 * @param nif_raw Output NIF buffer.
 * @param name_buf Output name buffer.
 * @param has_name Output flag for name.
 * @param valid_name Output flag for name validity.
 */
static void handle_unquoted_args(const char *ptr, char *nif_raw, char *name_buf,
                                 int *has_name, int *valid_name) {
  const char *start = ptr;
  while (*ptr && !isspace((unsigned char)*ptr))
    ptr++;
  size_t len = ptr - start;

  const char *after_w1 = ptr;
  while (*after_w1 && isspace((unsigned char)*after_w1))
    after_w1++;

  if (*after_w1 == '\0') {
    handle_f_single_word(start, len, nif_raw, name_buf, has_name);
  } else {
    handle_f_multi_word(start, len, after_w1, nif_raw, name_buf, has_name,
                        valid_name);
  }
}

/*
 * Parses arguments for the f command.
 * @param line Full line.
 * @param nif_raw Output NIF buffer.
 * @param name_buf Output name buffer.
 * @param has_name Output flag for name.
 * @param valid_name Output flag for name validity.
 */
static void parse_f_args(const char *line, char *nif_raw, char *name_buf,
                         int *has_name, int *valid_name) {
  *has_name = 0;
  *valid_name = 1;
  nif_raw[0] = '\0';
  name_buf[0] = '\0';
  const char *ptr = line;
  while (*ptr && isspace((unsigned char)*ptr))
    ptr++;
  if (!*ptr)
    return;

  if (*ptr == '"') {
    if (!extract_quoted_string(&ptr, name_buf, MAX_INSTRC_LENGTH))
      *valid_name = 0;
    *has_name = 1;
    while (*ptr && isspace((unsigned char)*ptr))
      ptr++;
    if (*ptr != '\0')
      *valid_name = 0;
    return;
  }

  handle_unquoted_args(ptr, nif_raw, name_buf, has_name, valid_name);
}

/*
 * Validates a NIF string.
 * @param nif_raw Raw NIF string.
 * @param nif Output nif integer.
 * @return Non-zero if valid.
 */
static int validate_f_nif(const char *nif_raw, int *nif) {
  int all_digits = 1;
  for (int i = 0; nif_raw[i]; i++) {
    if (!isdigit((unsigned char)nif_raw[i])) {
      all_digits = 0;
      break;
    }
  }
  if (!all_digits) {
    printf("%s: no such nif\n", nif_raw);
    return 0;
  }
  *nif = atoi(nif_raw);
  if (strlen(nif_raw) != 9 || *nif < 100000000 || *nif > 999999999) {
    printf("%s: no such nif\n", nif_raw);
    return 0;
  }
  return 1;
}

/*
 * Finalise the basket into an invoice.
 * @param sys System state.
 * @param table IVA rate table.
 */
void cmd_f(SystemState *sys, Iva table[]) {
  int nif = DEFAULT_NIF, has_name = 0, valid_name = 1;
  char name_buf[MAX_INSTRC_LENGTH] = "", nif_raw[MAX_INSTRC_LENGTH] = "";

  char *line = read_token_safe(sys);
  if (line) {
    parse_f_args(line, nif_raw, name_buf, &has_name, &valid_name);
    free_safe(line, strlen(line) + 1, sys);
  }

  if (nif_raw[0] != '\0') {
    if (!validate_f_nif(nif_raw, &nif))
      return;
  }

  if (has_name) {
    if (!valid_name || !is_valid_name_start(name_buf)) {
      printf("invalid name\n");
      return;
    }
  } else {
    strcpy(name_buf, "Cliente final");
  }

  if (strcmp(name_buf, "error") == 0) {
    cancel_basket(sys);
    return;
  }

  finalize_invoice(sys, table, nif, name_buf);
}
