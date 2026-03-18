/**
 * @file commands.c
 * @brief Command handlers for basket operations and client/invoice queries:
 *        @c a (add to basket), @c f (finalise invoice), @c c (list client
 *        invoices), and @c d (delete invoice or reduce stock).
 */

#include "commands.h"
#include "internal.h"
#include "memory.h"
#include "utils.h"

/* ── cmd_f helpers ──────────────────────────────────────────────────────────
 */

/**
 * @brief Copy the raw NIF digit string from @p line into @p nif_raw.
 *
 * @details Preserves leading zeros (e.g. @c "012345678") so that the error
 * message in cmd_f() can reproduce the original token exactly. Only copies
 * when the digit run is followed by whitespace or end-of-string.
 *
 * @param line    NUL-terminated argument string of the @c f command.
 * @param nif_raw Output buffer of at least ::BUFFER_LIMIT bytes.
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
  if (len >= BUFFER_LIMIT)
    len = BUFFER_LIMIT - 1;
  memcpy(nif_raw, ptr, len);
  nif_raw[len] = '\0';
}

/**
 * @brief Read and validate the arguments of the @c f command.
 *
 * @details Reads one line with read_token_safe(), captures the raw NIF
 * string for error reporting, calls parse_invoice_client() to extract the
 * NIF and name, then validates:
 * - A NIF with no following name is invalid.
 * - An unclosed quote is invalid.
 * - A name whose first character fails is_valid_name_start() is invalid.
 *
 * @param sys      System state.
 * @param name_buf Output buffer (::BUFFER_LIMIT bytes) for the client name.
 * @param nif_raw  Output buffer (::BUFFER_LIMIT bytes) for the raw NIF token.
 * @param nif      Receives the parsed NIF value.
 * @return Non-zero when arguments are valid, zero when @c "invalid name"
 *         should be printed.
 */
static int parse_f_args(SystemState *sys, char *name_buf, char *nif_raw,
                        int *nif) {
  char *line = read_token_safe(sys);
  if (!line)
    return 1; /* bare f — caller applies defaults */

  extract_nif_raw(line, nif_raw);
  int had_quote = (strchr(line, '"') != NULL);
  parse_invoice_client(line, nif, name_buf, BUFFER_LIMIT);
  int name_was_provided = (name_buf[0] != '\0');
  free_safe(line, strlen(line) + 1, sys);

  if (!name_was_provided && (nif_raw[0] != '\0' || had_quote))
    return 0;
  if (name_was_provided && !is_valid_name_start(name_buf))
    return 0;
  return 1;
}

/* ── cmd_c helpers ──────────────────────────────────────────────────────────
 */

/**
 * @brief Read and parse the optional client name argument of the @c c command.
 *
 * @details Handles both quoted (@c "Name With Spaces") and unquoted forms.
 * When the line is empty or absent, @p name is left as an empty string.
 *
 * @param sys  System state.
 * @param name Output buffer of at least ::BUFFER_LIMIT bytes.
 */
static void parse_c_name(SystemState *sys, char *name) {
  char *line = read_token_safe(sys);
  if (!line)
    return;
  char *ptr = line;
  while (*ptr && isspace((unsigned char)*ptr))
    ptr++;
  if (*ptr) {
    if (*ptr == '"') {
      ptr++;
      char *end = strchr(ptr, '"');
      if (end)
        *end = '\0';
      strncpy(name, ptr, BUFFER_LIMIT - 1);
    } else {
      strncpy(name, ptr, BUFFER_LIMIT - 1);
      char *end = name;
      while (*end && !isspace((unsigned char)*end))
        end++;
      *end = '\0';
    }
  }
  free_safe(line, strlen(line) + 1, sys);
}

/**
 * @brief Print all invoices of @p cr in chronological (FIFO) order.
 *
 * @details Format per line: @c "<id> <total> <client-name>"
 *
 * @param cr Client record whose invoice FIFO is to be printed.
 */
static void print_client_invoices(const ClientRecord *cr) {
  for (int ii = 0; ii < cr->invoice_count; ii++)
    printf("%d %.2f %s\n", cr->invoices[ii].id, cr->invoices[ii].total,
           cr->name);
}

/* ── public command handlers ────────────────────────────────────────────────
 */

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
      /* First token is a valid non-zero integer — treat it as quantity. */
      qty = (int)pq;
      strncpy(ean, s2, 13);
      ean[13] = '\0';
    } else {
      /* First token is not an integer — treat the whole thing as EAN, qty=1. */
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
  char name_buf[BUFFER_LIMIT] = "";
  char nif_raw[BUFFER_LIMIT] = "";

  if (!parse_f_args(sys, name_buf, nif_raw, &nif)) {
    printf("invalid name\n");
    return;
  }

  /* Apply the anonymous default client when no name was provided. */
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
  char name[BUFFER_LIMIT] = "";
  parse_c_name(sys, name);

  if (name[0] != '\0' && !is_valid_name_start(name)) {
    printf("invalid name\n");
    return;
  }

  if (name[0] == '\0') {
    /* No name given — print all clients in sorted order. */
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
  if (c == '\n' || c == EOF)
    return;

  char buf[MAX_INSTRC_LENGTH] = {0};
  int i = 0, truncated = 0;
  buf[i++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF) {
    if (c == '\r')
      continue;
    if (i < MAX_INSTRC_LENGTH - 1)
      buf[i++] = (char)c;
    else
      truncated = 1;
  }
  if (truncated)
    drain_line();
  buf[i] = '\0';

  char arg1[BUFFER_LIMIT], arg2[BUFFER_LIMIT];
  int n = sscanf(buf, "%1023s %1023s", arg1, arg2);

  if (n == 1)
    cmd_d_delete_inv(sys, atoi(arg1)); /* one arg  → delete invoice  */
  else if (n == 2)
    cmd_d_reduce_stock(sys, arg1, atoi(arg2)); /* two args → reduce stock  */
}