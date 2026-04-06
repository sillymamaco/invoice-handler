/*
 * Command 'c' implementation and parsing utilities.
 * @file c.c
 * @author IST1117890 (Irina Cojocari)
 */

#include "c.h"
#include "l.h"
#include "p.h"

/*
 * Read the rest of the current stdin line as a single token.
 * @param sys System state.
 * @return Dynamically allocated string, or NULL for a blank line.
 */
char *read_token_safe(SystemState *sys) {
  char buf[MAX_INSTRC_LENGTH] = {0};
  int c, i = 0, truncated = 0;

  while ((c = getchar()) != '\n' && c != EOF && isspace((unsigned char)c))
    ;
  if (c != '\n' && c != EOF) {
    buf[i++] = (char)c;
    fill_buffer_from_stdin(buf, &i, &truncated);
  }
  if (truncated) { /* if it exceeds the instruction limit, drain the line */
    while ((c = getchar()) != '\n' && c != EOF)
      ;
  }
  while (i > 0 && isspace((unsigned char)buf[i - 1]))
    i--;
  buf[i] = '\0';

  if (i == 0)
    return NULL;
  char *res = safemalloc(i + 1, sys);
  strcpy(res, buf);
  return res;
}

/*
 * Extract a string handling quotes.
 * @param ptr Pointer to input stream.
 * @param name_buf Output buffer.
 * @param name_buf_size Buffer size.
 * @return Non-zero on success.
 */
int extract_quoted_string(const char **ptr, char *name_buf,
                          size_t name_buf_size) {
  const char *p = *ptr;
  name_buf[0] = '\0';
  if (*p == '"') {
    p++;
    const char *end = strchr(p, '"');
    if (!end)
      return 0;
    size_t len = (size_t)(end - p);
    /* truncate the name */
    if (len >= name_buf_size)
      len = name_buf_size - 1;
    memcpy(name_buf, p, len);
    name_buf[len] = '\0';
    p = end + 1;
  } else {
    size_t i = 0;
    while (*p && !isspace((unsigned char)*p) && i < name_buf_size - 1)
      name_buf[i++] = *p++;
    name_buf[i] = '\0';
  }
  *ptr = p;
  return 1;
}

/*
 * Check that a client name starts with a valid character.
 * @param s Name string.
 * @return Non-zero if valid.
 */
int is_valid_name_start(const char *s) {
  if (!s || !s[0])
    return 0;
  unsigned char c = (unsigned char)s[0];
  if (isdigit(c))
    return 0;
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= 0xC0);
}

/*
 * Compare two client name strings.
 * @param a First name.
 * @param b Second name.
 * @return Comparison result.
 */
int cmp_names(const char *a, const char *b) { return strcmp(a, b); }

/*
 * bsearch comparator: client name key vs ClientRecord element.
 * @param key Pointer to a name string.
 * @param elem Pointer to a client record.
 * @return Comparison result.
 */
static int cmp_client_search(const void *key, const void *elem) {
  return cmp_names((const char *)key, ((const ClientRecord *)elem)->name);
}

/*
 * Binary-search the client array for a record by name.
 * @param sys System state.
 * @param name Client name.
 * @return Index or -1 if not found.
 */
int find_client_idx(SystemState *sys, const char *name) {
  if (sys->client_count == 0)
    return -1;
  ClientRecord *cr = bsearch(name, sys->clients, sys->client_count,
                             sizeof(ClientRecord), cmp_client_search);
  return cr ? (int)(cr - sys->clients) : -1;
}

/*
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

/*
 * Prints all invoices for a given client.
 * @param cr Client record.
 */
static void print_client_invoices(const ClientRecord *cr) {
  for (int ii = 0; ii < cr->invoice_count; ii++) {
    printf("%d %.2f %s\n", cr->invoices[ii].id,
           cr->invoices[ii].total_cents / 100.0, cr->name);
  }
}

/*
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
