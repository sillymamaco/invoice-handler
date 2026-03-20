/**
 * @file utils.c
 * @author IST1117890 (Irina Cojocari)
 * @brief Utility functions: input, validation, math, output, and parsing.
 */

#include "utils.h"
#include "memory.h"

char *read_token_safe(SystemState *sys) {
  char buf[MAX_INSTRC_LENGTH] = {0};
  int c, i = 0, truncated = 0;

  /* Skip leading whitespace but stop at newline so as to not miss other
  commands. */
  while ((c = getchar()) != '\n' && c != EOF && isspace((unsigned char)c))
    ;
  if (c != '\n' && c != EOF) {
    buf[i++] = (char)c;
    while ((c = getchar()) != '\n' && c != EOF) {
      if (c == '\r') /* return char */
        continue;
      if (i < MAX_INSTRC_LENGTH - 1)
        buf[i++] = (char)c;
      else
        truncated = 1; /* keep reading to drain the line */
    }
  }
  if (truncated) /* Get rid of trash */
    while ((c = getchar()) != '\n' && c != EOF)
      ;
  /* Strip trailing whitespace. */
  while (i > 0 && isspace((unsigned char)buf[i - 1]))
    i--;
  buf[i] = '\0';
  if (i == 0)
    return NULL;
  /* save the string */
  char *res = safemalloc(i + 1, sys);
  strcpy(res, buf);
  return res;
}

int cmp_product_search(const void *key, const void *elem) {
  return strcmp((const char *)key, ((const Product *)elem)->ean);
}

int find_product_idx(SystemState *sys, const char *ean) {
  if (sys->catalog_count == 0)
    return -1;
  Product *p = bsearch(ean, sys->catalog, sys->catalog_count, sizeof(Product),
                       cmp_product_search);
  /* if found, pointer - start = index */
  return p ? (int)(p - sys->catalog) : -1;
}

int iva_is_present(const Iva table[], char letter) {
  if (letter < 'A' || letter > 'Z')
    return 0;
  return table[letter - 'A'].present;
}

int get_iva_rate(const Iva table[], char letter) {
  if (!iva_is_present(table, letter))
    return 0;
  return table[letter - 'A'].value;
}

void iva_set(Iva table[], char letter, int value) {
  if (letter < 'A' || letter > 'Z')
    return;
  table[letter - 'A'].value = value;
  table[letter - 'A'].present = 1;
}

int validate_ean(const char *ean) {
  int len = (int)strlen(ean);
  if (len != 8 && len != 13)
    return 0;
  /* separated loops to ensure the last digit is validated before checking */
  for (int i = 0; i < len; i++)
    if (!isdigit((unsigned char)ean[i]))
      return 0;
  int sum = 0;
  for (int i = 0; i < len - 1; i++) {
    int val = ean[i] - '0';
    sum += (i % 2 == 0) ? val : 3 * val;
  }
  return ((10 - (sum % 10)) % 10) == (ean[len - 1] - '0');
}

int is_valid_desc_start(const char *s) {
  if (!s || !s[0])
    return 0;
  unsigned char c = (unsigned char)s[0];
  return (c >= 'A' && c <= 'Z') || (c >= 0xC0);
}

int is_valid_name_start(const char *s) {
  if (!s || !s[0])
    return 0;
  unsigned char c = (unsigned char)s[0];
  if (isdigit(c))
    return 0;
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= 0xC0);
}

int validate_p_input(const char *ean, int iva_ok, double price, int stock,
                     const char *desc) {
  if (!validate_ean(ean)) {
    printf("invalid ean\n");
    return 0;
  }
  if (!iva_ok) {
    printf("invalid iva\n");
    return 0;
  }
  if (price <= 0) {
    printf("invalid price\n");
    return 0;
  }
  if (stock < 0) {
    printf("invalid quantity\n");
    return 0;
  }
  if (!desc || !is_valid_desc_start(desc) || strlen(desc) > 50) {
    printf("invalid description\n");
    return 0;
  }
  return 1;
}

double round_money(double val) {
  /* imma be honest, by trial and error */
  return (long long)(val * 100.0 + 0.51) / 100.0;
}

int match(const char *pattern, const char *text) {
  const char *star = NULL, *ts = text;
  while (*text) {
    if (*pattern == '?' || *pattern == *text) {
      pattern++;
      text++;
    } else if (*pattern == '*') {
      star = pattern++;
      ts = text;
      /* if doesnt match but we have a star "to spare" */
    } else if (star) {
      pattern = star + 1; /* go back */
      text = ++ts;
    } else
      return 0;
  }
  while (*pattern == '*')
    pattern++;
  return *pattern == '\0';
}

void print_product(const Product *p) {
  printf("%s %c %.2f %d %d %s\n", p->ean, p->iva_class, p->price, p->sold,
         p->stock, p->desc);
}

void print_basket_item(SystemState *sys, const Iva table[], int cat_idx,
                       int qty) {
  double price = sys->catalog[cat_idx].price;
  int iva = get_iva_rate(table, sys->catalog[cat_idx].iva_class);
  double total = round_money((price * qty) * (1.0 + (iva / 100.0)));
  printf("%c %.2f %d %.2f %s\n", sys->catalog[cat_idx].iva_class, price, qty,
         total, sys->catalog[cat_idx].desc);
}

/**
 * @brief Parse a leading NIF from *ptr and advance past it.
 *
 * @details The digit run is treated as a NIF only when it is followed by
 * whitespace or end-of-string. A leading '0' sets *nif to  0 (so
 * the out-of-range check in cmd_f fires) without discarding the raw
 * string that is preserved for the error message.
 *
 * @param Pointer to the current parse position; advanced past
 *                    the NIF on success.
 * @param Receives the parsed NIF value.
 */
static void parse_nif(const char **ptr, int *nif) {
  if (!isdigit((unsigned char)**ptr))
    return;
  const char *tmp = *ptr;
  while (*tmp && isdigit((unsigned char)*tmp))
    tmp++;
  if (*tmp != '\0' && !isspace((unsigned char)*tmp))
    return;
  *nif = (**ptr == '0') ? 0 : (sscanf(*ptr, "%d", nif), *nif);
  *ptr = tmp;
}

/**
 * @brief Copy the name token starting at ptr into name_buf.
 *
 * @details Handles both quoted (white-space-containing) and unquoted names.
 * An unclosed quote leaves name_buf empty as an invalidity signal to the
 * caller.
 *
 * @param Parse position pointing at the first name character.
 * @param Caller-supplied destination buffer.
 * @param Size of name_buf in bytes.
 * @return Non-zero on success, zero when the opening quote has no closing
 *         counterpart.
 */
static int parse_name(const char **ptr_in, char *name_buf,
                      size_t name_buf_size) {
  const char *ptr = *ptr_in;
  name_buf[0] = '\0';
  if (*ptr == '"') {
    ptr++;
    const char *end = strchr(ptr, '"');
    if (!end)
      return 0; /* unclosed quote — signal invalidity */
    size_t len = (size_t)(end - ptr);
    if (len >= name_buf_size)
      len = name_buf_size - 1;
    memcpy(name_buf, ptr, len);
    name_buf[len] = '\0';
    ptr = end + 1;
  } else {
    size_t i = 0;
    while (*ptr && !isspace((unsigned char)*ptr) && i < name_buf_size - 1)
      name_buf[i++] = *ptr++;
    name_buf[i] = '\0';
  }
  *ptr_in = ptr;
  return 1;
}

int parse_invoice_client(const char *line, int *nif, char *name_buf,
                         size_t name_buf_size) {
  const char *ptr = line;
  while (*ptr && isspace((unsigned char)*ptr))
    ptr++;
  parse_nif(&ptr, nif);
  while (*ptr && isspace((unsigned char)*ptr))
    ptr++;
  if (*ptr) {
    if (!parse_name(&ptr, name_buf, name_buf_size))
      return 0;
    while (*ptr && isspace((unsigned char)*ptr))
      ptr++;
    if (*ptr != '\0')
      return 0; /* Trailing garbage detected */
  }
  return 1;
}
int cmp_names(const char *a, const char *b) { return strcmp(a, b); }

/**
 * @brief bsearch comparator: name key vs ClientRecord element.
 * @param Pointer to a const char* name string.
 * @param Pointer to a ClientRecord entry.
 * @return Result of cmp_names() on the two name strings.
 */
static int cmp_client_search(const void *key, const void *elem) {
  return cmp_names((const char *)key, ((const ClientRecord *)elem)->name);
}

int find_client_idx(SystemState *sys, const char *name) {
  if (sys->client_count == 0)
    return -1;
  ClientRecord *cr = bsearch(name, sys->clients, sys->client_count,
                             sizeof(ClientRecord), cmp_client_search);
  /* index = client pointer - start */
  return cr ? (int)(cr - sys->clients) : -1;
}
