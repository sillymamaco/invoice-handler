/*
 * Command 'p' implementation and core memory/product utilities.
 * @file p.c
 * @author IST1117890 (Irina Cojocari)
 */

#include "p.h"
#include "r.h"

/*
 * Decrement the memory counter then free a previously tracked block.
 * @param pointer Pointer to the block to free.
 * @param size Byte count originally allocated.
 * @param sys System state.
 */
void free_safe(void *pointer, size_t size, SystemState *sys) {
  if (pointer) {
    sys->memory_used = (sys->memory_used >= size) ? sys->memory_used - size : 0;
    free(pointer);
  }
}

/*
 * Release every heap allocation owned by sys.
 * @param sys System state to clean up.
 */
void clean_all(SystemState *sys) {
  /* clean the basket */
  if (sys->basket) {
    free_safe(sys->basket, sys->basket_capacity * sizeof(BasketItem), sys);
    sys->basket = NULL;
  }
  /* clean the registration of clients and their respective invoices */
  if (sys->clients) {
    for (int i = 0; i < sys->client_count; i++) {
      ClientRecord *cr = &sys->clients[i];
      if (cr->invoices)
        free_safe(cr->invoices, cr->invoice_cap * sizeof(Invoice), sys);
      if (cr->name)
        free_safe(cr->name, strlen(cr->name) + 1, sys);
    }
    free_safe(sys->clients, sys->client_capacity * sizeof(ClientRecord), sys);
    sys->clients = NULL;
  }
  /* clean the catalog */
  if (sys->catalog) {
    for (int i = 0; i < sys->catalog_count; i++) {
      if (sys->catalog[i].desc)
        free_safe(sys->catalog[i].desc, strlen(sys->catalog[i].desc) + 1, sys);
    }
    free_safe(sys->catalog, sys->catalog_capacity * sizeof(Product), sys);
    sys->catalog = NULL;
  }
}

/*
 * Print "No memory." and terminate program cleanly.
 * @param sys System state.
 */
static void no_memory(SystemState *sys) {
  printf("No memory.\n");
  clean_all(sys);
  exit(0);
}

/*
 * Allocate size bytes and track the allocation in sys.
 * @param size Number of bytes to allocate.
 * @param sys System state.
 * @return Pointer to the newly allocated block.
 */
void *safemalloc(size_t size, SystemState *sys) {
  if (size == 0)
    return NULL;
  /* check if there's memory */
  void *p = malloc(size);
  if (!p)
    no_memory(sys);

  sys->memory_used += size;
  return p;
}

/*
 * Resize a tracked allocation.
 * @param pointer Pointer to the block to resize.
 * @param old_size Current size of the block.
 * @param new_size Desired size of the block.
 * @param sys System state.
 * @return Pointer to the resized block.
 */
void *safe_realloc(void *pointer, size_t old_size, size_t new_size,
                   SystemState *sys) {
  void *n = realloc(pointer, new_size);
  if (!n && new_size > 0)
    no_memory(sys);

  if (new_size >= old_size)
    sys->memory_used += (new_size - old_size);
  else
    sys->memory_used -= (old_size - new_size);

  return n;
}

/*
 * bsearch comparator: EAN string key vs Product element.
 * @param key Pointer to an EAN string.
 * @param elem Pointer to a product.
 * @return Comparison result.
 */
int cmp_product_search(const void *key, const void *elem) {
  return strcmp((const char *)key, ((const Product *)elem)->ean);
}

/*
 * Binary-search the catalog for a product by EAN.
 * @param sys System state.
 * @param ean EAN string to find.
 * @return Index or -1 if not found.
 */
int find_product_idx(SystemState *sys, const char *ean) {
  if (sys->catalog_count == 0)
    return -1;
  Product *p = bsearch(ean, sys->catalog, sys->catalog_count, sizeof(Product),
                       cmp_product_search);
  /* subtract the catalog start pointer to get the index from the pointer */
  return p ? (int)(p - sys->catalog) : -1;
}

/*
 * Validate an EAN code.
 * @param ean EAN string.
 * @return Non-zero if valid.
 */
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

/*
 * Check that a description starts with a valid character.
 * @param s Description string.
 * @return Non-zero if valid.
 */
int is_valid_desc_start(const char *s) {
  if (!s || !s[0])
    return 0;
  unsigned char c = (unsigned char)s[0];
  return (c >= 'A' && c <= 'Z') || (c >= 0xC0);
}

/*
 * Validate fields of a p command.
 * @param ean EAN string.
 * @param iva_ok IVA validity flag.
 * @param price Parsed price.
 * @param stock Parsed quantity.
 * @param desc Description string.
 * @return Non-zero if valid.
 */
static int validate_p_input(const char *ean, int iva_ok, double price,
                            int stock, const char *desc) {
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

/*
 * Read one command argument line from stdin into buf.
 * @param buf Destination buffer.
 * @param bufsz Size of buf in bytes.
 * @return Non-zero if characters were read.
 */
static int read_p_line(char *buf, int bufsz) {
  int c;
  while ((c = getchar()) == ' ' || c == '\t')
    ;
  if (c == '\n' || c == EOF)
    return 0;

  int li = 0;
  buf[li++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF) {
    if (li < bufsz - 1)
      buf[li++] = (char)c;
  }
  buf[li] = '\0';
  return 1;
}

/*
 * Skips the first four tokens of a p line to find the description.
 * @param dp Pointer to the line string.
 * @return Pointer to the description start.
 */
static const char *skip_p_tokens(const char *dp) {
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
  return dp;
}

/*
 * Return a copy of the product description.
 * @param line Full argument line.
 * @param sys System state.
 * @return Description string or NULL.
 */
static char *extract_desc(const char *line, SystemState *sys) {
  const char *dp = skip_p_tokens(line);
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

/*
 * Insert a new product into the catalog.
 * @param sys System state.
 * @param ean EAN string.
 * @param iva_c IVA letter.
 * @param price Unit price.
 * @param stock Initial stock.
 * @param desc Description.
 */
static void catalog_insert(SystemState *sys, const char *ean, char iva_c,
                           double price, int stock, char *desc) {
  if (sys->catalog_count == sys->catalog_capacity) {
    int nc = sys->catalog_capacity ? sys->catalog_capacity * 2 : 10;
    sys->catalog =
        safe_realloc(sys->catalog, sys->catalog_capacity * sizeof(Product),
                     nc * sizeof(Product), sys);
    sys->catalog_capacity = nc;
  }
  /* insert in the right place to avoid sorting later */
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

/*
 * Update an existing product.
 * @param sys System state.
 * @param idx Catalog index.
 * @param ean Product EAN.
 * @param iva_c New IVA letter.
 * @param price New price.
 * @param stock Stock to add.
 * @param desc New description.
 * @return Non-zero on success.
 */
static int catalog_update(SystemState *sys, int idx, const char *ean,
                          char iva_c, double price, int stock, char *desc) {
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

/*
 * Parses a price string, ensuring it's not in scientific notation.
 * @param price_str String to parse.
 * @return Parsed price or -1.0 on error.
 */
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

/*
 * Parses a stock string, ensuring it is a positive integer.
 * @param stock_str String to parse.
 * @return Parsed stock or -1 on error.
 */
static int parse_stock_field(const char *stock_str) {
  char *se = NULL;
  long sl = strtol(stock_str, &se, 10);
  if (se == stock_str || *se != '\0' || stock_str[0] == '+')
    return -1;
  return (int)sl;
}

/*
 * Parse fields from a p command line.
 * @param linebuf Full argument line.
 * @param ean Output buffer for EAN.
 * @param iva_c Receives IVA letter.
 * @param price Receives price.
 * @param stock Receives quantity.
 * @return Non-zero when tokens are successfully parsed.
 */
static int parse_p_fields(const char *linebuf, char ean[14], char *iva_c,
                          double *price, int *stock) {
  char ean_raw[14] = {0}, iva_str[16] = {0}, price_str[32] = {0},
       stock_str[32] = {0};

  if (sscanf(linebuf, "%14s %15s %31s %31s", ean_raw, iva_str, price_str,
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

/*
 * Handles internal logic for p command (update or insert).
 * @param sys System state.
 * @param ean EAN string.
 * @param iva_c IVA letter.
 * @param price Price.
 * @param stock Stock.
 * @param desc Description.
 */
static void handle_catalog_update_insert(SystemState *sys, const char *ean,
                                         char iva_c, double price, int stock,
                                         char *desc) {
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

/**
 * Insert or update a product in the catalog.
 * @param sys System state.
 * @param table IVA rate table.
 */
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

  handle_catalog_update_insert(sys, ean, iva_c, price, stock, desc);
}
