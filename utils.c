/**
 * @file utils.c
 * @author IST1117890 (Irina Cojocari)
 * @brief Utility functions: input, validation, math, output, and parsing.
 */

#include "utils.h"
#include "memory.h"

void fill_buffer_from_stdin(char *buf, int *i, int *truncated) {
  int c;
  while ((c = getchar()) != '\n' && c != EOF) {
    if (c == '\r')
      continue;
    if (*i < MAX_INSTRC_LENGTH - 1)
      buf[(*i)++] = (char)c;
    else
      *truncated = 1;
  }
}

int read_line_to_buffer(char *buf, size_t limit) {
  int c;
  int i = 0, truncated = 0;
  while ((c = getchar()) == ' ' || c == '\t' || c == '\r')
    ;
  if (c == '\n' || c == EOF)
    return 0;

  buf[i++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF) {
    if (c == '\r')
      continue;
    if (i < (int)limit - 1)
      buf[i++] = (char)c;
    else
      truncated = 1;
  }
  if (truncated) {
    while ((c = getchar()) != '\n' && c != EOF)
      ;
  }
  buf[i] = '\0';
  return truncated;
}

char *read_token_safe(SystemState *sys) {
  char buf[MAX_INSTRC_LENGTH] = {0};
  int c, i = 0, truncated = 0;

  while ((c = getchar()) != '\n' && c != EOF && isspace((unsigned char)c))
    ;
  if (c != '\n' && c != EOF) {
    buf[i++] = (char)c;
    fill_buffer_from_stdin(buf, &i, &truncated);
  }
  if (truncated) {
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

int cmp_product_search(const void *key, const void *elem) {
  return strcmp((const char *)key, ((const Product *)elem)->ean);
}

int find_product_idx(SystemState *sys, const char *ean) {
  if (sys->catalog_count == 0)
    return -1;
  Product *p = bsearch(ean, sys->catalog, sys->catalog_count, sizeof(Product),
                       cmp_product_search);
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
  for (int i = 0; i < len; i++) {
    if (!isdigit((unsigned char)ean[i]))
      return 0;
  }
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
  return (long long)(val * 100.0 + 0.500000001) / 100.0;
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
    } else if (star) {
      pattern = star + 1;
      text = ++ts;
    } else {
      return 0;
    }
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
  long long price_cents = (long long)(price * 100.0 + 0.5);
  long long numerator = price_cents * qty * (100 + iva);
  long long total_cents = (numerator + 50) / 100;

  printf("%c %.2f %d %.2f %s\n", sys->catalog[cat_idx].iva_class, price, qty,
         total_cents / 100.0, sys->catalog[cat_idx].desc);
}

int cmp_names(const char *a, const char *b) { return strcmp(a, b); }

static int cmp_client_search(const void *key, const void *elem) {
  return cmp_names((const char *)key, ((const ClientRecord *)elem)->name);
}

int find_client_idx(SystemState *sys, const char *name) {
  if (sys->client_count == 0)
    return -1;
  ClientRecord *cr = bsearch(name, sys->clients, sys->client_count,
                             sizeof(ClientRecord), cmp_client_search);
  return cr ? (int)(cr - sys->clients) : -1;
}