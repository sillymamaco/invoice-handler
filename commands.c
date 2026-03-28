/**
 * Handlers for basket add/remove, invoice finalisation, client lists.
 * @file commands.c
 * @author IST1117890 (Irina Cojocari)
 */

#include "commands.h"
#include "memory.h"
#include "shared.h"
#include "utils.h"

/**
 * Parses the client name for the c command.
 * @param sys System state.
 * @param name Output name buffer.
 * @return Non-zero on success.
 */
static int parse_c_name(SystemState *sys, char *name) {
  char *line = read_token_safe(sys);
  if (!line)
    return 1;

  const char *ptr = line;
  while (*ptr && isspace((unsigned char)*ptr))
    ptr++;

  if (*ptr) {
    if (!extract_quoted_string(&ptr, name, MAX_INSTRC_LENGTH)) {
      free_safe(line, strlen(line) + 1, sys);
      return 0;
    }
    while (*ptr && isspace((unsigned char)*ptr))
      ptr++;
    if (*ptr != '\0') {
      free_safe(line, strlen(line) + 1, sys);
      return 0;
    }
  }
  free_safe(line, strlen(line) + 1, sys);
  return 1;
}

/**
 * Prints all invoices for a given client.
 * @param cr Client record.
 */
static void print_client_invoices(const ClientRecord *cr) {
  for (int ii = 0; ii < cr->invoice_count; ii++) {
    printf("%d %.2f %s\n", cr->invoices[ii].id,
           cr->invoices[ii].total_cents / 100.0, cr->name);
  }
}

/**
 * Add a quantity of a product to the basket.
 * @param sys System state.
 * @param table IVA rate table.
 */
void cmd_a(SystemState *sys, Iva table[]) {
  char buf[BUFFER_LIMIT] = {0};

  if (!read_line_to_buffer(buf, BUFFER_LIMIT) && buf[0] == '\0') {
    print_sorted_basket(sys, table);
    return;
  }

  char s1[BUFFER_LIMIT], s2[14];
  char ean[14] = {0};
  int qty = 1;

  if (sscanf(buf, "%1023s %13s", s1, s2) == 2) {
    char *endp = NULL;
    long pq = strtol(s1, &endp, 10);
    if (endp != s1 && *endp == '\0') {
      qty = (int)pq;
      strncpy(ean, s2, 13);
      ean[13] = '\0';
    } else {
      strncpy(ean, s1, 13);
      ean[13] = '\0';
    }
  } else {
    strncpy(ean, s1, 13);
    ean[13] = '\0';
  }
  process_basket_add(sys, table, ean, qty);
}

/**
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

/**
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

/**
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

/**
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

/**
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

/**
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

/**
 * List invoices for a client or all clients.
 * @param sys System state.
 */
void cmd_c(SystemState *sys) {
  char name[MAX_INSTRC_LENGTH] = "";
  if (!parse_c_name(sys, name) ||
      (name[0] != '\0' && !is_valid_name_start(name))) {
    printf("invalid name\n");
    return;
  }
  if (name[0] == '\0') {
    for (int ci = 0; ci < sys->client_count; ci++)
      print_client_invoices(&sys->clients[ci]);
    return;
  }
  int ci = find_client_idx(sys, name);
  if (ci == -1 || sys->clients[ci].invoice_count == 0) {
    printf("%s: no such client\n", name);
    return;
  }
  print_client_invoices(&sys->clients[ci]);
}

/**
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
