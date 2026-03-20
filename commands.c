/**
 * @file commands.c
 * @brief Handlers for a (basket add/remove), f (finalise invoice),
 * c (list client invoices), and d (delete invoice or reduce stock).
 */

#include "commands.h"
#include "memory.h"
#include "shared.h"
#include "utils.h"

/** @brief Copy the raw NIF digit string from @p line into @p nif_raw,
 *  preserving leading zeros for accurate error messages. Only copies when
 *  the digit run is followed by whitespace or end-of-string.
 *  @param line NUL-terminated f command argument. @param nif_raw Output buffer.
 */
static void extract_nif_raw(const char *line, char *nif_raw) {
  const char *ptr = line;
  while (*ptr && isspace((unsigned char)*ptr))
    ptr++;
  if (!isdigit((unsigned char)*ptr))
    return;
  const char *tmp = ptr;
  while (*tmp && isdigit((unsigned char)*tmp))
    tmp++;
  if (*tmp != '\0' && !isspace((unsigned char)*tmp))
    return;
  size_t len = (size_t)(tmp - ptr);
  if (len >= MAX_INSTRC_LENGTH)
    len = MAX_INSTRC_LENGTH - 1;
  memcpy(nif_raw, ptr, len);
  nif_raw[len] = '\0';
}

/** @brief Read and validate f command arguments: NIF (optional) and client
 *  name. Returns 0 and signals "invalid name" when: a NIF has no name,
 *  a quote is unclosed, or the name fails is_valid_name_start().
 *  @param sys System state. @param name_buf Output name buffer.
 *  @param nif_raw Output raw NIF string. @param nif Receives parsed NIF. */
static int parse_f_args(SystemState *sys, char *name_buf, char *nif_raw,
                        int *nif) {
  char *line = read_token_safe(sys);
  if (!line)
    return 1;

  extract_nif_raw(line, nif_raw);
  int had_quote = (strchr(line, '"') != NULL);
  int valid_syntax =
      parse_invoice_client(line, nif, name_buf, MAX_INSTRC_LENGTH);
  int name_was_provided = (name_buf[0] != '\0');
  free_safe(line, strlen(line) + 1, sys);

  if (!valid_syntax)
    return 0;
  if (!name_was_provided && (nif_raw[0] != '\0' || had_quote))
    return 0;
  if (name_was_provided && !is_valid_name_start(name_buf))
    return 0;
  return 1;
}

/** @brief Read the optional client name argument for the c command, handling
 *  both quoted and unquoted forms. Leaves @p name empty when absent.
 *  @param sys System state. @param name Output buffer. */
static int parse_c_name(SystemState *sys, char *name) {
  char *line = read_token_safe(sys);
  if (!line)
    return 1;
  char *ptr = line;
  while (*ptr && isspace((unsigned char)*ptr))
    ptr++;
  if (*ptr) {
    if (*ptr == '"') {
      ptr++;
      char *end = strchr(ptr, '"');
      if (!end) {
        free_safe(line, strlen(line) + 1, sys);
        return 0;
      }
      *end = '\0';
      strncpy(name, ptr, MAX_INSTRC_LENGTH - 1);
      ptr = end + 1;
    } else {
      char *start = ptr;
      while (*ptr && !isspace((unsigned char)*ptr))
        ptr++;
      size_t len = (size_t)(ptr - start);
      if (len >= MAX_INSTRC_LENGTH)
        len = MAX_INSTRC_LENGTH - 1;
      memcpy(name, start, len);
      name[len] = '\0';
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

/** @brief Print all invoices for @p cr in chronological order.
 *  Format: "<id> <total> <client-name>" */
static void print_client_invoices(const ClientRecord *cr) {
  for (int ii = 0; ii < cr->invoice_count; ii++)
    printf("%d %.2f %s\n", cr->invoices[ii].id, cr->invoices[ii].total,
           cr->name);
}

void cmd_a(SystemState *sys, Iva table[]) {
  int c;
  while ((c = getchar()) == ' ' || c == '\t' || c == '\r')
    ;
  if (c == '\n' || c == EOF) {
    print_sorted_basket(sys, table);
    return;
  }

  char buf[BUFFER_LIMIT] = {0};
  int i = 0, truncated = 0;
  buf[i++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF) {
    if (c == '\r')
      continue;
    if (i < BUFFER_LIMIT - 1)
      buf[i++] = (char)c;
    else
      truncated = 1;
  }
  if (truncated)
    drain_line();
  buf[i] = '\0';

  char s1[BUFFER_LIMIT], s2[14];
  char ean[14] = {0};
  int qty = 1;

  if (sscanf(buf, "%1023s %13s", s1, s2) == 2) {
    char *endp = NULL;
    long pq = strtol(s1, &endp, 10);
    if (endp != s1 && *endp == '\0' && pq != 0) {
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

void cmd_f(SystemState *sys, Iva table[]) {
  int nif = 999999999;
  char name_buf[MAX_INSTRC_LENGTH] = "";
  char nif_raw[MAX_INSTRC_LENGTH] = "";

  if (!parse_f_args(sys, name_buf, nif_raw, &nif)) {
    printf("invalid name\n");
    return;
  }
  if (name_buf[0] == '\0')
    strcpy(name_buf, "Cliente final");

  if (nif != 999999999 && (nif < 100000000 || nif > 999999999)) {
    printf("%s: no such nif\n", nif_raw[0] ? nif_raw : "0");
    return;
  }
  if (strcmp(name_buf, "error") == 0) {
    cancel_basket(sys);
    return;
  }

  finalize_invoice(sys, table, nif, name_buf);
}

void cmd_c(SystemState *sys) {
  char name[MAX_INSTRC_LENGTH] = "";

  if (!parse_c_name(sys, name)) {
    printf("invalid name\n");
    return;
  }

  if (name[0] != '\0' && !is_valid_name_start(name)) {
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
  int c;
  while ((c = getchar()) == ' ' || c == '\t' || c == '\r')
    ;
  if (c == '\n' || c == EOF) return;

  char buf[MAX_INSTRC_LENGTH] = {0};
  int i = 0, truncated = 0;
  buf[i++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF) {
    if (c == '\r') continue;
    if (i < MAX_INSTRC_LENGTH - 1) buf[i++] = (char)c;
    else truncated = 1;
  }
  if (truncated) drain_line();
  buf[i] = '\0';

  char arg1[BUFFER_LIMIT], arg2[BUFFER_LIMIT];
  int n = sscanf(buf, "%1023s %1023s", arg1, arg2);
  if (n == 1)
    cmd_d_delete_inv(sys, atoi(arg1));
  else if (n == 2) {
    if (strlen(arg1) > 13) printf("invalid ean\n");
    else cmd_d_reduce_stock(sys, arg1, atoi(arg2));
  }
}