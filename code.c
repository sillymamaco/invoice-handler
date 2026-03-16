/* iaed26 - ist1117890 - project */
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MEMORY_ALLOCATED (4 * 1024 * 1024)
#define MAX_INSTRC_LENGTH 65535
#define BUFFER_LIMIT 1024

/* DATA STRUCTURES */
typedef struct {
  int value;
  char letter;
} Iva;

typedef struct {
  char *desc;
  double price;
  int stock;
  int sold;
  char ean[14];
  uint32_t hash; /* NOVO: Hash super rápido para pesquisas */
  char iva_class;
} Product;

typedef struct {
  char *client_name;
  double total;
  int nif;
  int id;
  int num_items;
} Invoice;

typedef struct {
  char ean[14];
  int amount;
  uint32_t hash; /* NOVO: O cesto também guarda o hash */
} BasketItem;

typedef struct {
  Product *catalog;
  int catalog_count;
  int catalog_capacity;
  Invoice *history;
  int history_count;
  int history_capacity;
  BasketItem *basket;
  int basket_count;
  int basket_capacity;
  int memory_used;
  int next_invoice_id;
  int global_items;
  double global_sales;
} SystemState;

/* MEMORY HANDLING */
void free_safe(void *pointer, size_t size, SystemState *sys) {
  if (pointer) {
    sys->memory_used -= (int)size;
    free(pointer);
  }
}

void clean_all(SystemState *sys) {
  if (sys->basket)
    free_safe(sys->basket, sys->basket_capacity * sizeof(BasketItem), sys);
  if (sys->history) {
    for (int i = 0; i < sys->history_count; i++)
      if (sys->history[i].client_name)
        free_safe(sys->history[i].client_name,
                  strlen(sys->history[i].client_name) + 1, sys);
    free_safe(sys->history, sys->history_capacity * sizeof(Invoice), sys);
  }
  if (sys->catalog) {
    for (int i = 0; i < sys->catalog_count; i++)
      if (sys->catalog[i].desc)
        free_safe(sys->catalog[i].desc, strlen(sys->catalog[i].desc) + 1, sys);
    free_safe(sys->catalog, sys->catalog_capacity * sizeof(Product), sys);
  }
}

void no_memory(SystemState *sys) {
  printf("No memory.\n");
  clean_all(sys);
  exit(0);
}

void *safemalloc(size_t size, SystemState *sys) {
  if ((sys->memory_used + (int)size) > MAX_MEMORY_ALLOCATED)
    no_memory(sys);
  void *p = malloc(size);
  if (p)
    sys->memory_used += (int)size;
  return p;
}

void *safe_realloc(void *pointer, size_t old_size, size_t new_size,
                   SystemState *sys) {
  int diff = (int)new_size - (int)old_size;
  if ((sys->memory_used + diff) > MAX_MEMORY_ALLOCATED)
    no_memory(sys);
  void *n = realloc(pointer, new_size);
  if (n)
    sys->memory_used += diff;
  return n;
}

char *read_token_safe(SystemState *sys) {
  char buf[MAX_INSTRC_LENGTH] = {0};
  int c, i = 0;
  while ((c = getchar()) != '\n' && c != EOF && isspace(c))
    ;
  if (c != '\n' && c != EOF) {
    buf[i++] = (char)c;
    while ((c = getchar()) != '\n' && c != EOF)
      if (i < MAX_INSTRC_LENGTH && c != '\r')
        buf[i++] = (char)c;
  }
  while (i > 0 && isspace((unsigned char)buf[i - 1]))
    i--;
  buf[i] = '\0';
  if (i == 0)
    return NULL;
  char *res = safemalloc(i + 1, sys);
  if (res)
    strcpy(res, buf);
  return res;
}

/* ==============================================
   FAST HASHING & UTILS
   ============================================== */

/* Algoritmo DJB2: Transforma strings em inteiros únicos super rápido */
uint32_t get_hash(const char *str) {
  uint32_t hash = 5381;
  int c;
  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;
  return hash;
}

/* Agora a pesquisa procura pelo Hash primeiro (1 ciclo de CPU) */
int find_product_idx(SystemState *sys, const char *ean, uint32_t h) {
  for (int i = 0; i < sys->catalog_count; i++)
    if (sys->catalog[i].hash == h && strcmp(sys->catalog[i].ean, ean) == 0)
      return i;
  return -1;
}

int get_iva_rate(Iva table[], int iva_count, char iva_class) {
  for (int i = 0; i < iva_count; i++)
    if (table[i].letter == iva_class)
      return table[i].value;
  return 0;
}

int validate_ean(const char *ean) {
  int sum = 0, len = (int)strlen(ean);
  if (len != 8 && len != 13)
    return 0;
  for (int i = 0; i < len - 1; i++) {
    int val = ean[i] - '0';
    sum += (i % 2 == 0) ? val : 3 * val;
  }
  return (((10 - (sum % 10)) % 10) == (ean[len - 1] - '0'));
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
    } else
      return 0;
  }
  while (*pattern == '*')
    pattern++;
  return *pattern == '\0';
}

int is_valid_desc_start(const char *s) {
  if (!s || !s[0])
    return 0;
  unsigned char c = (unsigned char)s[0];
  if (c >= 'A' && c <= 'Z')
    return 1;
  if (c >= 0xC0)
    return 1;
  return 0;
}

int is_valid_name_start(const char *s) {
  if (!s || !s[0])
    return 0;
  unsigned char c = (unsigned char)s[0];
  if (c >= 'A' && c <= 'Z')
    return 1;
  if (c >= 'a' && c <= 'z')
    return 1;
  if (c >= 0xC0)
    return 1;
  return 0;
}

double round_money(double val) {
  return (long long)(val * 100.0 + 0.500000001) / 100.0;
}

void print_product(Product *p) {
  printf("%s %c %.2f %d %d %s\n", p->ean, p->iva_class, p->price, p->sold,
         p->stock, p->desc);
}

void print_basket_item(SystemState *sys, Iva table[], int iva_count,
                       int cat_idx, int qty) {
  double price = sys->catalog[cat_idx].price;
  int iva = get_iva_rate(table, iva_count, sys->catalog[cat_idx].iva_class);
  double total = (price * qty) * (1.0 + (iva / 100.0));
  printf("%c %.2f %d %.2f %s\n", sys->catalog[cat_idx].iva_class, price, qty,
         round_money(total), sys->catalog[cat_idx].desc);
}

void cancel_basket(SystemState *sys) {
  for (int i = 0; i < sys->basket_count; i++) {
    int cat_idx =
        find_product_idx(sys, sys->basket[i].ean, sys->basket[i].hash);
    if (cat_idx != -1) {
      sys->catalog[cat_idx].stock += sys->basket[i].amount;
      sys->catalog[cat_idx].sold -= sys->basket[i].amount;
    }
  }
  sys->basket_count = 0;
}

/* MERGE SORTS */
void merge_invoices(Invoice **arr, int l, int m, int r) {
  int i, j, k, n1 = m - l + 1, n2 = r - m;
  Invoice **L = malloc(n1 * sizeof(Invoice *));
  Invoice **R = malloc(n2 * sizeof(Invoice *));
  for (i = 0; i < n1; i++)
    L[i] = arr[l + i];
  for (j = 0; j < n2; j++)
    R[j] = arr[m + 1 + j];
  i = 0;
  j = 0;
  k = l;
  while (i < n1 && j < n2) {
    int cmp = strcmp(L[i]->client_name, R[j]->client_name);
    if (cmp < 0 || (cmp == 0 && L[i]->id < R[j]->id))
      arr[k++] = L[i++];
    else
      arr[k++] = R[j++];
  }
  while (i < n1)
    arr[k++] = L[i++];
  while (j < n2)
    arr[k++] = R[j++];
  free(L);
  free(R);
}

void merge_sort_invoices(Invoice **arr, int l, int r) {
  if (l < r) {
    int m = l + (r - l) / 2;
    merge_sort_invoices(arr, l, m);
    merge_sort_invoices(arr, m + 1, r);
    merge_invoices(arr, l, m, r);
  }
}

void merge_basket(BasketItem *arr, int l, int m, int r) {
  int i, j, k, n1 = m - l + 1, n2 = r - m;
  BasketItem *L = malloc(n1 * sizeof(BasketItem));
  BasketItem *R = malloc(n2 * sizeof(BasketItem));
  for (i = 0; i < n1; i++)
    L[i] = arr[l + i];
  for (j = 0; j < n2; j++)
    R[j] = arr[m + 1 + j];
  i = 0;
  j = 0;
  k = l;
  while (i < n1 && j < n2) {
    if (strcmp(L[i].ean, R[j].ean) <= 0)
      arr[k++] = L[i++];
    else
      arr[k++] = R[j++];
  }
  while (i < n1)
    arr[k++] = L[i++];
  while (j < n2)
    arr[k++] = R[j++];
  free(L);
  free(R);
}

void merge_sort_basket(BasketItem *arr, int l, int r) {
  if (l < r) {
    int m = l + (r - l) / 2;
    merge_sort_basket(arr, l, m);
    merge_sort_basket(arr, m + 1, r);
    merge_basket(arr, l, m, r);
  }
}

/* COMMAND AUXILIARIES */
int validate_p_input(const char *ean, int iva_ok, double price, int stock,
                     const char *desc) {
  int desc_valid = desc && is_valid_desc_start(desc) && strlen(desc) <= 50;
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
  if (!desc || !desc_valid) {
    printf("invalid description\n");
    return 0;
  }
  return 1;
}

void print_sorted_basket(SystemState *sys, Iva table[], int iva_count) {
  if (sys->basket_count > 1)
    merge_sort_basket(sys->basket, 0, sys->basket_count - 1);
  for (int i = 0; i < sys->basket_count; i++) {
    int cat_idx =
        find_product_idx(sys, sys->basket[i].ean, sys->basket[i].hash);
    if (cat_idx != -1)
      print_basket_item(sys, table, iva_count, cat_idx, sys->basket[i].amount);
  }
}

void process_basket_add(SystemState *sys, Iva table[], int iva_count,
                        const char *ean, int qty) {
  uint32_t h = get_hash(ean);
  int cat_idx = find_product_idx(sys, ean, h), basket_idx = -1;
  for (int j = 0; j < sys->basket_count; j++) {
    /* Fast path com hash! */
    if (sys->basket[j].hash == h && strcmp(sys->basket[j].ean, ean) == 0) {
      basket_idx = j;
      break; /* Para imediatamente a procura */
    }
  }

  if (cat_idx == -1) {
    printf("%s: no such product\n", ean);
    return;
  }
  if (qty > sys->catalog[cat_idx].stock ||
      (qty < 0 &&
       (basket_idx == -1 || sys->basket[basket_idx].amount + qty < 0))) {
    printf("no stock\n");
    return;
  }

  sys->catalog[cat_idx].stock -= qty;
  sys->catalog[cat_idx].sold += qty;
  if (basket_idx != -1) {
    sys->basket[basket_idx].amount += qty;
  } else {
    if (sys->basket_count == sys->basket_capacity) {
      int new_cap = sys->basket_capacity ? sys->basket_capacity * 2 : 10;
      sys->basket =
          safe_realloc(sys->basket, sys->basket_capacity * sizeof(BasketItem),
                       new_cap * sizeof(BasketItem), sys);
      sys->basket_capacity = new_cap;
    }
    basket_idx = sys->basket_count++;
    strcpy(sys->basket[basket_idx].ean, ean);
    sys->basket[basket_idx].hash = h;
    sys->basket[basket_idx].amount = qty;
  }
  print_basket_item(sys, table, iva_count, cat_idx,
                    sys->basket[basket_idx].amount);
}

void parse_invoice_client(char *line, int *nif, char **name) {
  char *ptr = line;
  while (*ptr && isspace(*ptr))
    ptr++;
  if (isdigit(*ptr)) {
    sscanf(ptr, "%d", nif);
    while (*ptr && !isspace(*ptr))
      ptr++;
  }
  while (*ptr && isspace(*ptr))
    ptr++;
  if (*ptr) {
    if (*ptr == '"') {
      ptr++;
      char *end = strchr(ptr, '"');
      if (end) {
        *end = '\0';
        *name = ptr;
      } else {
        *name = "1_invalid";
      }
    } else {
      *name = ptr;
      char *end = *name;
      while (*end && !isspace((unsigned char)*end))
        end++;
      *end = '\0';
    }
  }
}

void finalize_invoice(SystemState *sys, Iva table[], int iva_count, int nif,
                      const char *name) {
  double total = 0;
  int items = 0;
  for (int i = 0; i < sys->basket_count; i++) {
    int cat_idx =
        find_product_idx(sys, sys->basket[i].ean, sys->basket[i].hash);
    if (cat_idx != -1 && sys->basket[i].amount > 0) {
      items += sys->basket[i].amount;
      int iva = get_iva_rate(table, iva_count, sys->catalog[cat_idx].iva_class);
      double sub = (sys->catalog[cat_idx].price * sys->basket[i].amount) *
                   (1.0 + (iva / 100.0));
      total += round_money(sub);
    }
  }
  if (sys->history_count == sys->history_capacity) {
    int new_cap = sys->history_capacity ? sys->history_capacity * 2 : 10;
    sys->history =
        safe_realloc(sys->history, sys->history_capacity * sizeof(Invoice),
                     new_cap * sizeof(Invoice), sys);
    sys->history_capacity = new_cap;
  }
  int h_idx = sys->history_count++;
  sys->history[h_idx].nif = nif;
  sys->history[h_idx].total = total;
  sys->history[h_idx].id = sys->next_invoice_id++;
  sys->history[h_idx].num_items = items;
  sys->history[h_idx].client_name = safemalloc(strlen(name) + 1, sys);
  strcpy(sys->history[h_idx].client_name, name);

  sys->global_items += items;
  sys->global_sales += total;
  printf("%d %.2f %d\n", items, total, sys->history[h_idx].id);
  sys->basket_count = 0;
}

void cmd_d_delete_inv(SystemState *sys, int inv_id) {
  int h_idx = -1;
  for (int j = 0; j < sys->history_count; j++)
    if (sys->history[j].id == inv_id) {
      h_idx = j;
      break;
    }
  if (h_idx == -1) {
    printf("%d: no such invoice\n", inv_id);
    return;
  }

  printf("%.2f %d %s\n", sys->history[h_idx].total, sys->history[h_idx].nif,
         sys->history[h_idx].client_name);
  sys->global_items -= sys->history[h_idx].num_items;
  sys->global_sales -= sys->history[h_idx].total;
  free_safe(sys->history[h_idx].client_name,
            strlen(sys->history[h_idx].client_name) + 1, sys);

  for (int j = h_idx; j < sys->history_count - 1; j++)
    sys->history[j] = sys->history[j + 1];
  sys->history_count--;
}

void cmd_d_reduce_stock(SystemState *sys, const char *ean, int qty) {
  uint32_t h = get_hash(ean);
  int cat_idx = find_product_idx(sys, ean, h);
  if (cat_idx == -1) {
    printf("%s: no such product\n", ean);
    return;
  }

  int will_delete = (sys->catalog[cat_idx].stock <= qty);
  int in_basket = 0;
  for (int j = 0; j < sys->basket_count; j++) {
    if (sys->basket[j].hash == h && strcmp(sys->basket[j].ean, ean) == 0) {
      in_basket = 1;
      break;
    }
  }

  if (will_delete && in_basket) {
    printf("product in use\n");
    return;
  }

  if (qty <= 0 || qty > sys->catalog[cat_idx].stock) {
    printf("invalid quantity\n");
    return;
  }

  sys->catalog[cat_idx].stock -= qty;
  if (sys->catalog[cat_idx].stock == 0) {
    printf("0 %s\n", sys->catalog[cat_idx].desc);
    free_safe(sys->catalog[cat_idx].desc,
              strlen(sys->catalog[cat_idx].desc) + 1, sys);
    for (int j = cat_idx; j < sys->catalog_count - 1; j++)
      sys->catalog[j] = sys->catalog[j + 1];
    sys->catalog_count--;
  } else {
    printf("%d %s\n", sys->catalog[cat_idx].stock, sys->catalog[cat_idx].desc);
  }
}

/* MAIN COMMANDS */
void cmd_p(SystemState *sys, Iva table[], int iva_count) {
  char ean[14], iva_c;
  double price;
  int stock;
  if (scanf("%s %c %lf %d", ean, &iva_c, &price, &stock) != 4)
    return;
  char *desc = read_token_safe(sys);

  int iva_ok = 0;
  for (int i = 0; i < iva_count; i++)
    if (table[i].letter == iva_c)
      iva_ok = 1;
  if (!validate_p_input(ean, iva_ok, price, stock, desc)) {
    if (desc)
      free_safe(desc, strlen(desc) + 1, sys);
    return;
  }

  uint32_t h = get_hash(ean);
  int idx = find_product_idx(sys, ean, h);
  if (idx != -1) {
    for (int i = 0; i < sys->basket_count; i++) {
      if (sys->basket[i].hash == h && strcmp(sys->basket[i].ean, ean) == 0 &&
          sys->catalog[idx].price != price) {
        printf("product in use\n");
        free_safe(desc, strlen(desc) + 1, sys);
        return;
      }
    }
    sys->catalog[idx].iva_class = iva_c;
    sys->catalog[idx].price = price;
    sys->catalog[idx].stock += stock;
    free_safe(sys->catalog[idx].desc, strlen(sys->catalog[idx].desc) + 1, sys);
    sys->catalog[idx].desc = desc;
  } else {
    if (sys->catalog_count == sys->catalog_capacity) {
      int new_cap = sys->catalog_capacity ? sys->catalog_capacity * 2 : 10;
      sys->catalog =
          safe_realloc(sys->catalog, sys->catalog_capacity * sizeof(Product),
                       new_cap * sizeof(Product), sys);
      sys->catalog_capacity = new_cap;
    }
    idx = sys->catalog_count++;
    strcpy(sys->catalog[idx].ean, ean);
    sys->catalog[idx].hash = h;
    sys->catalog[idx].iva_class = iva_c;
    sys->catalog[idx].price = price;
    sys->catalog[idx].stock = stock;
    sys->catalog[idx].desc = desc;
    sys->catalog[idx].sold = 0;
  }
  printf("%d\n", sys->catalog[idx].stock);
}

void cmd_l(SystemState *sys) {
  int c;
  while ((c = getchar()) == ' ' || c == '\t' || c == '\r')
    ;
  if (c == '\n' || c == EOF) {
    int found = 0;
    for (int i = 0; i < sys->catalog_count; i++) {
      if (sys->catalog[i].stock > 0) {
        print_product(&sys->catalog[i]);
        found = 1;
      }
    }
    if (!found)
      printf("*: no such product\n");
    return;
  }

  char buf[MAX_INSTRC_LENGTH] = {0};
  int i = 0;
  buf[i++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF)
    if (i < MAX_INSTRC_LENGTH && c != '\r')
      buf[i++] = (char)c;
  buf[i] = '\0';

  char *token = strtok(buf, " \t\r\n");
  while (token) {
    int found_any = 0;
    int has_wildcard =
        (strchr(token, '*') != NULL || strchr(token, '?') != NULL);

    /* FAST PATH: Se não tem wildcards, procura por Hash instantaneamente */
    if (!has_wildcard) {
      uint32_t h = get_hash(token);
      int idx = find_product_idx(sys, token, h);
      if (idx != -1 && sys->catalog[idx].stock > 0) {
        print_product(&sys->catalog[idx]);
        found_any = 1;
      }
    } else {
      /* SLOW PATH: Lógica original para testar os padroes */
      for (int j = 0; j < sys->catalog_count; j++) {
        if (match(token, sys->catalog[j].ean)) {
          if (sys->catalog[j].stock > 0) {
            print_product(&sys->catalog[j]);
            found_any = 1;
          }
        }
      }
    }
    if (!found_any)
      printf("%s: no such product\n", token);
    token = strtok(NULL, " \t\r\n");
  }
}

void cmd_a(SystemState *sys, Iva table[], int iva_count) {
  int c;
  while ((c = getchar()) == ' ' || c == '\t' || c == '\r')
    ;
  if (c == '\n' || c == EOF) {
    print_sorted_basket(sys, table, iva_count);
    return;
  }

  char buf[BUFFER_LIMIT] = {0};
  int i = 0;
  buf[i++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF)
    if (i < BUFFER_LIMIT && c != '\r')
      buf[i++] = (char)c;
  buf[i] = '\0';

  char s1[BUFFER_LIMIT], s2[BUFFER_LIMIT], ean[14];
  int qty = 1;
  if (sscanf(buf, "%s %s", s1, s2) == 2) {
    qty = atoi(s1);
    strcpy(ean, s2);
  } else {
    strcpy(ean, s1);
  }

  process_basket_add(sys, table, iva_count, ean, qty);
}

void cmd_f(SystemState *sys, Iva table[], int iva_count) {
  char *line = read_token_safe(sys);
  int nif = 999999999;
  char *name = "Cliente final";

  if (line)
    parse_invoice_client(line, &nif, &name);
  if (nif != 999999999 && (nif < 100000000 || nif > 999999999)) {
    printf("%d: no such nif\n", nif);
    if (line)
      free_safe(line, strlen(line) + 1, sys);
    return;
  }
  if (strcmp(name, "error") == 0) {
    cancel_basket(sys);
    if (line)
      free_safe(line, strlen(line) + 1, sys);
    return;
  }
  if (!is_valid_name_start(name) && strcmp(name, "Cliente final") != 0) {
    printf("invalid name\n");
    if (line)
      free_safe(line, strlen(line) + 1, sys);
    return;
  }

  finalize_invoice(sys, table, iva_count, nif, name);
  if (line)
    free_safe(line, strlen(line) + 1, sys);
}

void cmd_r(SystemState *sys, Iva table[], int iva_count) {
  int c;
  while ((c = getchar()) == ' ' || c == '\t' || c == '\r')
    ;
  if (c == '\n' || c == EOF) {
    printf("%d %d %.2f\n", sys->global_items, sys->next_invoice_id - 1,
           sys->global_sales);
    Iva sorted_table[26];
    memcpy(sorted_table, table, iva_count * sizeof(Iva));
    for (int i = 1; i < iva_count; i++) {
      Iva key = sorted_table[i];
      int j = i - 1;
      while (j >= 0 && sorted_table[j].letter > key.letter) {
        sorted_table[j + 1] = sorted_table[j];
        j--;
      }
      sorted_table[j + 1] = key;
    }
    for (int i = 0; i < iva_count; i++)
      printf("%c %d%%\n", sorted_table[i].letter, sorted_table[i].value);
    return;
  }

  char ean[14];
  ean[0] = (char)c;
  scanf("%s", ean + 1);
  uint32_t h = get_hash(ean);
  int cat_idx = find_product_idx(sys, ean, h);
  if (cat_idx == -1)
    printf("%s: no such product\n", ean);
  else
    printf("%d %d %s\n", sys->catalog[cat_idx].stock,
           sys->catalog[cat_idx].sold, sys->catalog[cat_idx].desc);
}

void cmd_c(SystemState *sys) {
  char *line = read_token_safe(sys);
  char name[BUFFER_LIMIT] = "";
  if (line) {
    char *ptr = line;
    while (*ptr && isspace(*ptr))
      ptr++;
    if (*ptr) {
      if (*ptr == '"') {
        ptr++;
        char *end = strchr(ptr, '"');
        if (end)
          *end = '\0';
        strcpy(name, ptr);
      } else {
        strcpy(name, ptr);
        char *end = name;
        while (*end && !isspace((unsigned char)*end))
          end++;
        *end = '\0';
      }
    }
  }

  if (strlen(name) == 0) {
    Invoice **sorted_refs =
        safemalloc(sys->history_count * sizeof(Invoice *), sys);
    for (int i = 0; i < sys->history_count; i++)
      sorted_refs[i] = &sys->history[i];
    if (sys->history_count > 0)
      merge_sort_invoices(sorted_refs, 0, sys->history_count - 1);
    for (int i = 0; i < sys->history_count; i++)
      printf("%d %.2f %s\n", sorted_refs[i]->id, sorted_refs[i]->total,
             sorted_refs[i]->client_name);
    free_safe(sorted_refs, sys->history_count * sizeof(Invoice *), sys);
  } else {
    int found = 0;
    for (int i = 0; i < sys->history_count; i++) {
      if (strcmp(sys->history[i].client_name, name) == 0) {
        printf("%d %.2f %s\n", sys->history[i].id, sys->history[i].total,
               sys->history[i].client_name);
        found = 1;
      }
    }
    if (!found)
      printf("%s: no such client\n", name);
  }
  if (line)
    free_safe(line, strlen(line) + 1, sys);
}

void cmd_d(SystemState *sys) {
  int c;
  while ((c = getchar()) == ' ' || c == '\t' || c == '\r')
    ;
  if (c == '\n' || c == EOF)
    return;

  char buf[MAX_INSTRC_LENGTH] = {0};
  int i = 0;
  buf[i++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF)
    if (i < MAX_INSTRC_LENGTH && c != '\r')
      buf[i++] = (char)c;
  buf[i] = '\0';

  char arg1[BUFFER_LIMIT], arg2[BUFFER_LIMIT];
  int num_args = sscanf(buf, "%1023s %1023s", arg1, arg2);

  if (num_args == 1)
    cmd_d_delete_inv(sys, atoi(arg1));
  else if (num_args == 2)
    cmd_d_reduce_stock(sys, arg1, atoi(arg2));
}

int main(int argc, char *argv[]) {
  SystemState sys = {0};
  sys.next_invoice_id = 1;
  Iva iva_table[26];
  int iva_count = 0;

  if (argc == 1) {
    iva_table[0] = (Iva){0, 'A'};
    iva_table[1] = (Iva){6, 'B'};
    iva_table[2] = (Iva){13, 'C'};
    iva_table[3] = (Iva){23, 'D'};
    iva_count = 4;
  } else {
    FILE *f = fopen(argv[1], "r");
    if (f) {
      while (fscanf(f, " %c %d", &iva_table[iva_count].letter,
                    &iva_table[iva_count].value) == 2)
        iva_count++;
      fclose(f);
    }
  }

  int command;
  while ((command = getchar()) != EOF) {
    if (isspace(command))
      continue;
    switch (command) {
    case 'q':
      clean_all(&sys);
      return 0;
    case 'p':
      cmd_p(&sys, iva_table, iva_count);
      break;
    case 'l':
      cmd_l(&sys);
      break;
    case 'a':
      cmd_a(&sys, iva_table, iva_count);
      break;
    case 'r':
      cmd_r(&sys, iva_table, iva_count);
      break;
    case 'f':
      cmd_f(&sys, iva_table, iva_count);
      break;
    case 'c':
      cmd_c(&sys);
      break;
    case 'd':
      cmd_d(&sys);
      break;
    }
  }
  clean_all(&sys);
  return 0;
}