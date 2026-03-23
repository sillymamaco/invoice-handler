/**
 * @file commands.c
 * @author IST1117890 (Irina Cojocari)
 * @brief Handlers for basket add/remove, invoice finalisation, client lists.
 */

#include "commands.h"
#include "memory.h"
#include "shared.h"
#include "utils.h"

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

static void print_client_invoices(const ClientRecord *cr) {
  for (int ii = 0; ii < cr->invoice_count; ii++) {
    printf("%d %.2f %s\n", cr->invoices[ii].id,
           cr->invoices[ii].total_cents / 100.0, cr->name);
  }
}

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

static int parse_f_nif(const char **ptr, int *nif, char *nif_raw) {
  if (isdigit((unsigned char)**ptr)) {
    const char *tmp = *ptr;
    while (*tmp && isdigit((unsigned char)*tmp))
      tmp++;
    if (*tmp == '\0' || isspace((unsigned char)*tmp)) {
      size_t len = tmp - *ptr;
      if (len >= MAX_INSTRC_LENGTH)
        len = MAX_INSTRC_LENGTH - 1;
      strncpy(nif_raw, *ptr, len);
      nif_raw[len] = '\0';

      if (nif_raw[0] == '0')
        *nif = 0;
      else
        sscanf(nif_raw, "%d", nif);
      *ptr = tmp;
      return 1;
    }
  }
  return 0;
}

static int parse_f_name(const char **ptr, char *name_buf, int *has_name) {
  if (**ptr) {
    *has_name = 1;
    if (!extract_quoted_string(ptr, name_buf, MAX_INSTRC_LENGTH))
      return 0;
    while (**ptr && isspace((unsigned char)**ptr))
      (*ptr)++;
    if (**ptr != '\0')
      return 0;
  }
  return 1;
}

/**
 * @brief Validates NIF and Name for the f command.
 * @param nif The numeric NIF.
 * @param nif_raw String representation of NIF.
 * @param name_buf Name string.
 * @param has_name Flag if name was provided.
 * @param valid_name Flag if name parsing succeeded.
 * @return Non-zero if valid.
 */
static int validate_invoice_client_data(int nif, const char *nif_raw,
                                        const char *name_buf, int has_name,
                                        int valid_name) {
  if (nif_raw[0] != '\0') {
    if (strlen(nif_raw) != 9 || nif < 100000000 || nif > 999999999) {
      printf("%s: no such nif\n", nif_raw);
      return 0;
    }
  }
  if (has_name && (!valid_name || !is_valid_name_start(name_buf))) {
    printf("invalid name\n");
    return 0;
  }
  return 1;
}

void cmd_f(SystemState *sys, Iva table[]) {
  int nif = DEFAULT_NIF, has_name = 0;
  char name_buf[MAX_INSTRC_LENGTH] = "", nif_raw[MAX_INSTRC_LENGTH] = "";

  char *line = read_token_safe(sys);
  if (!line) {
    finalize_invoice(sys, table, DEFAULT_NIF, "Cliente final");
    return;
  }

  const char *ptr = line;
  while (*ptr && isspace((unsigned char)*ptr))
    ptr++;

  parse_f_nif(&ptr, &nif, nif_raw);
  while (*ptr && isspace((unsigned char)*ptr))
    ptr++;

  int valid_name = parse_f_name(&ptr, name_buf, &has_name);
  free_safe(line, strlen(line) + 1, sys);

  if (!validate_invoice_client_data(nif, nif_raw, name_buf, has_name,
                                    valid_name))
    return;
  if (!has_name)
    strcpy(name_buf, "Cliente final");
  if (strcmp(name_buf, "error") == 0) {
    cancel_basket(sys);
    return;
  }

  finalize_invoice(sys, table, nif, name_buf);
}

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