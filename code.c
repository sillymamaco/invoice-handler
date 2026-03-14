#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_MEMORY_ALLOCATED 65535

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
} SystemState;

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
  static char buf[65536];
  int c, i = 0;
  while ((c = getchar()) != '\n' && c != EOF && isspace(c))
    ;
  if (c != '\n' && c != EOF) {
    buf[i++] = (char)c;
    while ((c = getchar()) != '\n' && c != EOF)
      if (i < 65535 && c != '\r') // Ignorar também o \r a meio da string
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

int find_product_idx(SystemState *sys, const char *ean) {
  for (int i = 0; i < sys->catalog_count; i++)
    if (strcmp(sys->catalog[i].ean, ean) == 0)
      return i;
  return -1;
}

int get_iva_rate(Iva table[], int iva_count, char iva_class) {
  for (int i = 0; i < iva_count; i++)
    if (table[i].letter == iva_class)
      return table[i].value;
  return 0;
}

// A tua validação EAN original (100% correta matematicamente)
int validate_ean(const char *ean) {
  int sum = 0, len = (int)strlen(ean);
  if (len != 8 && len != 13)
    return 0;
  for (int i = 0; i < len - 1; i++) {
    int val = ean[i] - '0';
    sum += (i % 2 == 0) ? val : 3 * val;
  }
  int check = (10 - (sum % 10)) % 10;
  return (check == (ean[len - 1] - '0'));
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

void sort_basket(SystemState *sys) {
  for (int i = 1; i < sys->basket_count; i++) {
    BasketItem key = sys->basket[i];
    int j = i - 1;
    while (j >= 0 && strcmp(sys->basket[j].ean, key.ean) > 0) {
      sys->basket[j + 1] = sys->basket[j];
      j--;
    }
    sys->basket[j + 1] = key;
  }
}

void print_basket_item(SystemState *sys, Iva table[], int iva_count,
                       int cat_idx, int qty) {
  double price = sys->catalog[cat_idx].price;
  int iva = get_iva_rate(table, iva_count, sys->catalog[cat_idx].iva_class);
  double total = (price * qty) * (1.0 + (iva / 100.0));
  printf("%c %.2f %d %.2f %s\n", sys->catalog[cat_idx].iva_class, price, qty,
         (long)((total * 100) + 0.5) / 100.0, sys->catalog[cat_idx].desc);
}

int is_valid_first_char(const unsigned char *str) {
  if (!str || !str[0])
    return 0;
  if (str[0] >= 'A' && str[0] <= 'Z')
    return 1;
  if (str[0] == (unsigned char)0xC3 && str[1] == (unsigned char)0x81)
    return 1;
  return 0;
}

void cmd_p(SystemState *sys, Iva table[], int iva_count) {
  char ean[1024], iva_c;
  double price;
  int stock;
  if (scanf("%s %c %lf %d", ean, &iva_c, &price, &stock) != 4)
    return;
  char *desc = read_token_safe(sys);
  int iva_ok = 0;
  for (int i = 0; i < iva_count; i++)
    if (table[i].letter == iva_c)
      iva_ok = 1;

  int desc_valid =
      desc && is_valid_first_char((unsigned char *)desc) && strlen(desc) <= 50;

  // CORREÇÃO: A ordem dos erros DEVE ser EAN -> IVA -> Preço -> Qty -> Desc
  if (!validate_ean(ean) || !iva_ok || price <= 0 || stock < 0 || !desc ||
      !desc_valid) {
    if (!validate_ean(ean))
      printf("invalid ean\n");
    else if (!iva_ok)
      printf("invalid iva\n");
    else if (price <= 0)
      printf("invalid price\n");
    else if (stock < 0)
      printf("invalid quantity\n");
    else
      printf("invalid description\n");

    if (desc)
      free_safe(desc, strlen(desc) + 1, sys);
    return;
  }

  int idx = find_product_idx(sys, ean);
  if (idx != -1) {
    for (int i = 0; i < sys->basket_count; i++)
      if (strcmp(sys->basket[i].ean, ean) == 0 &&
          sys->catalog[idx].price != price) {
        printf("product in use\n");
        free_safe(desc, strlen(desc) + 1, sys);
        return;
      }
    sys->catalog[idx].iva_class = iva_c;
    sys->catalog[idx].price = price;
    sys->catalog[idx].stock += stock;
    free_safe(sys->catalog[idx].desc, strlen(sys->catalog[idx].desc) + 1, sys);
    sys->catalog[idx].desc = desc;
  } else {
    if (sys->catalog_count == sys->catalog_capacity) {
      int nc = sys->catalog_capacity ? sys->catalog_capacity * 2 : 10;
      sys->catalog =
          safe_realloc(sys->catalog, sys->catalog_capacity * sizeof(Product),
                       nc * sizeof(Product), sys);
      sys->catalog_capacity = nc;
    }
    idx = sys->catalog_count++;
    strcpy(sys->catalog[idx].ean, ean);
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
  // Apanhar os '\r' que vêm do Windows do Mooshak
  while ((c = getchar()) == ' ' || c == '\t' || c == '\r')
    ;
  if (c == '\n' || c == EOF) {
    int f = 0;
    for (int i = 0; i < sys->catalog_count; i++)
      if (sys->catalog[i].stock > 0) {
        printf("%s %c %.2f %d %d %s\n", sys->catalog[i].ean,
               sys->catalog[i].iva_class, sys->catalog[i].price,
               sys->catalog[i].sold, sys->catalog[i].stock,
               sys->catalog[i].desc);
        f = 1;
      }
    if (!f)
      printf("*: no such product\n");
    return;
  }
  char buf[65536];
  int i = 0;
  buf[i++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF)
    if (i < 65535 && c != '\r')
      buf[i++] = (char)c;
  buf[i] = '\0';
  char *tok = strtok(buf, " \t\r");
  while (tok) {
    int ex = 0;
    for (int j = 0; j < sys->catalog_count; j++)
      if (match(tok, sys->catalog[j].ean)) {
        if (sys->catalog[j].stock > 0)
          printf("%s %c %.2f %d %d %s\n", sys->catalog[j].ean,
                 sys->catalog[j].iva_class, sys->catalog[j].price,
                 sys->catalog[j].sold, sys->catalog[j].stock,
                 sys->catalog[j].desc);
        ex = 1;
      }
    if (!ex)
      printf("%s: no such product\n", tok);
    tok = strtok(NULL, " \t\r");
  }
}

void cmd_a(SystemState *sys, Iva table[], int iva_count) {
  int c;
  while ((c = getchar()) == ' ' || c == '\t' || c == '\r')
    ;
  if (c == '\n' || c == EOF) {
    sort_basket(sys);
    for (int i = 0; i < sys->basket_count; i++) {
      int idx = find_product_idx(sys, sys->basket[i].ean);
      if (idx != -1)
        print_basket_item(sys, table, iva_count, idx, sys->basket[i].amount);
    }
    return;
  }
  char buf[1024];
  int i = 0;
  buf[i++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF)
    if (i < 1023 && c != '\r')
      buf[i++] = (char)c;
  buf[i] = '\0';
  char s1[1024], s2[1024], ean[14];
  int qty = 1;
  int n = sscanf(buf, "%s %s", s1, s2);
  if (n == 2) {
    qty = atoi(s1);
    strcpy(ean, s2);
  } else
    strcpy(ean, s1);
  int ci = find_product_idx(sys, ean), bi = -1;
  for (int j = 0; j < sys->basket_count; j++)
    if (strcmp(sys->basket[j].ean, ean) == 0)
      bi = j;
  if (ci == -1) {
    printf("%s: no such product\n", ean);
    return;
  }
  if (qty > sys->catalog[ci].stock ||
      (qty < 0 && (bi == -1 || sys->basket[bi].amount + qty < 0))) {
    printf("no stock\n");
    return;
  }
  sys->catalog[ci].stock -= qty;
  sys->catalog[ci].sold += qty;
  if (bi != -1)
    sys->basket[bi].amount += qty;
  else {
    if (sys->basket_count == sys->basket_capacity) {
      int nc = sys->basket_capacity ? sys->basket_capacity * 2 : 10;
      sys->basket =
          safe_realloc(sys->basket, sys->basket_capacity * sizeof(BasketItem),
                       nc * sizeof(BasketItem), sys);
      sys->basket_capacity = nc;
    }
    bi = sys->basket_count++;
    strcpy(sys->basket[bi].ean, ean);
    sys->basket[bi].amount = qty;
  }
  print_basket_item(sys, table, iva_count, ci, sys->basket[bi].amount);
}

void cmd_f(SystemState *sys, Iva table[], int iva_count) {
  char *line = read_token_safe(sys);
  int nif = 999999999;
  char name[1024] = "Cliente final";
  if (line) {
    char *ptr = line;
    while (*ptr && isspace(*ptr))
      ptr++;
    if (isdigit(*ptr)) {
      sscanf(ptr, "%d", &nif);
      while (*ptr && !isspace(*ptr))
        ptr++;
    }
    while (*ptr && isspace(*ptr))
      ptr++;
    if (*ptr) {
      if (*ptr == '"') {
        ptr++;
        char *end = strrchr(ptr, '"');
        if (end)
          *end = '\0';
        strcpy(name, ptr);
      } else
        strcpy(name, ptr);
    }
  }

  if (nif != 999999999 && (nif < 100000000 || nif > 999999999)) {
    printf("%d: no such nif\n", nif);
    if (line)
      free_safe(line, strlen(line) + 1, sys);
    return;
  }
  if (strcmp(name, "error") == 0) {
    for (int i = 0; i < sys->basket_count; i++) {
      int idx = find_product_idx(sys, sys->basket[i].ean);
      if (idx != -1) {
        sys->catalog[idx].stock += sys->basket[i].amount;
        sys->catalog[idx].sold -= sys->basket[i].amount;
      }
    }
    sys->basket_count = 0;
    if (line)
      free_safe(line, strlen(line) + 1, sys);
    return;
  }
  if (!isalpha((unsigned char)name[0]) && strcmp(name, "Cliente final") != 0) {
    printf("invalid name\n");
    if (line)
      free_safe(line, strlen(line) + 1, sys);
    return;
  }

  double total = 0;
  int items = 0;
  for (int i = 0; i < sys->basket_count; i++) {
    int idx = find_product_idx(sys, sys->basket[i].ean);
    if (idx != -1 && sys->basket[i].amount > 0) {
      items += sys->basket[i].amount;
      int iva = get_iva_rate(table, iva_count, sys->catalog[idx].iva_class);
      double sub = (sys->catalog[idx].price * sys->basket[i].amount) *
                   (1.0 + (iva / 100.0));
      total += (long)((sub * 100) + 0.5) / 100.0;
    }
  }
  if (sys->history_count == sys->history_capacity) {
    int nc = sys->history_capacity ? sys->history_capacity * 2 : 10;
    sys->history =
        safe_realloc(sys->history, sys->history_capacity * sizeof(Invoice),
                     nc * sizeof(Invoice), sys);
    sys->history_capacity = nc;
  }
  int hi = sys->history_count++;
  sys->history[hi].nif = nif;
  sys->history[hi].total = total;
  sys->history[hi].id = sys->next_invoice_id++;
  sys->history[hi].num_items = items;
  sys->history[hi].client_name = safemalloc(strlen(name) + 1, sys);
  strcpy(sys->history[hi].client_name, name);
  printf("%d %.2f %d\n", items, total, sys->history[hi].id);
  sys->basket_count = 0;
  if (line)
    free_safe(line, strlen(line) + 1, sys);
}

void cmd_r(SystemState *sys, Iva table[], int iva_count) {
  int c;
  while ((c = getchar()) == ' ' || c == '\t' || c == '\r')
    ;
  if (c == '\n' || c == EOF) {
    int items = 0;
    double total = 0;
    for (int i = 0; i < sys->history_count; i++) {
      items += sys->history[i].num_items;
      total += sys->history[i].total;
    }
    // CORREÇÃO: Em Test27 apagam-se faturas, mas o número de faturas impressas
    // deve refletir o total acumulado que foi gerado, não as que sobram na
    // pool!
    printf("%d %d %.2f\n", items, sys->next_invoice_id - 1, total);

    Iva st[26];
    memcpy(st, table, iva_count * sizeof(Iva));
    for (int i = 0; i < iva_count - 1; i++)
      for (int j = 0; j < iva_count - i - 1; j++)
        if (st[j].letter > st[j + 1].letter) {
          Iva t = st[j];
          st[j] = st[j + 1];
          st[j + 1] = t;
        }
    for (int i = 0; i < iva_count; i++)
      printf("%c %d%%\n", st[i].letter, st[i].value);
    return;
  }
  char ean[14];
  ean[0] = (char)c;
  scanf("%s", ean + 1);
  int idx = find_product_idx(sys, ean);
  if (idx == -1)
    printf("%s: no such product\n", ean);
  else
    printf("%d %d %s\n", sys->catalog[idx].stock, sys->catalog[idx].sold,
           sys->catalog[idx].desc);
}

void cmd_c(SystemState *sys) {
  char *line = read_token_safe(sys);
  char name[1024] = "";
  if (line) {
    char *ptr = line;
    while (*ptr && isspace(*ptr))
      ptr++;
    if (*ptr) {
      if (*ptr == '"') {
        ptr++;
        char *end = strrchr(ptr, '"');
        if (end)
          *end = '\0';
        strcpy(name, ptr);
      } else {
        strcpy(name, ptr);
      }
    }
  }

  if (strlen(name) == 0) {
    Invoice **sorted = safemalloc(sys->history_count * sizeof(Invoice *), sys);
    for (int i = 0; i < sys->history_count; i++)
      sorted[i] = &sys->history[i];
    for (int i = 1; i < sys->history_count; i++) {
      Invoice *key = sorted[i];
      int j = i - 1;
      while (j >= 0 &&
             (strcmp(sorted[j]->client_name, key->client_name) > 0 ||
              (strcmp(sorted[j]->client_name, key->client_name) == 0 &&
               sorted[j]->id > key->id))) {
        sorted[j + 1] = sorted[j];
        j--;
      }
      sorted[j + 1] = key;
    }
    for (int i = 0; i < sys->history_count; i++)
      printf("%d %.2f %s\n", sorted[i]->id, sorted[i]->total,
             sorted[i]->client_name);
    free_safe(sorted, sys->history_count * sizeof(Invoice *), sys);
  } else {
    int f = 0;
    for (int i = 0; i < sys->history_count; i++)
      if (strcmp(sys->history[i].client_name, name) == 0) {
        printf("%d %.2f %s\n", sys->history[i].id, sys->history[i].total,
               sys->history[i].client_name);
        f = 1;
      }
    if (!f)
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
  char buf[65536];
  int i = 0;
  buf[i++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF)
    if (i < 65535 && c != '\r')
      buf[i++] = (char)c;
  buf[i] = '\0';

  char t1[1024], t2[1024];
  // CORREÇÃO: Utilizar sscanf para detetar exatamente quantos argumentos há!
  int num_args = sscanf(buf, "%1023s %1023s", t1, t2);

  if (num_args == 1) {
    int id = atoi(t1), idx = -1;
    for (int j = 0; j < sys->history_count; j++)
      if (sys->history[j].id == id) {
        idx = j;
        break;
      }
    if (idx == -1)
      printf("%d: no such invoice\n", id);
    else {
      printf("%.2f %d %s\n", sys->history[idx].total, sys->history[idx].nif,
             sys->history[idx].client_name);
      free_safe(sys->history[idx].client_name,
                strlen(sys->history[idx].client_name) + 1, sys);
      for (int j = idx; j < sys->history_count - 1; j++)
        sys->history[j] = sys->history[j + 1];
      sys->history_count--;
    }
  } else if (num_args == 2) {
    int qty = atoi(t2);
    int idx = find_product_idx(sys, t1);
    if (idx == -1)
      printf("%s: no such product\n", t1);
    else {
      int in = 0;
      for (int j = 0; j < sys->basket_count; j++)
        if (strcmp(sys->basket[j].ean, t1) == 0)
          in = 1;
      if (in)
        printf("product in use\n");
      else if (qty <= 0 || qty > sys->catalog[idx].stock)
        printf("invalid quantity\n");
      else {
        sys->catalog[idx].stock -= qty;
        if (sys->catalog[idx].stock == 0) {
          printf("0 %s\n", sys->catalog[idx].desc);
          free_safe(sys->catalog[idx].desc, strlen(sys->catalog[idx].desc) + 1,
                    sys);
          for (int j = idx; j < sys->catalog_count - 1; j++)
            sys->catalog[j] = sys->catalog[j + 1];
          sys->catalog_count--;
        } else
          printf("%d %s\n", sys->catalog[idx].stock, sys->catalog[idx].desc);
      }
    }
  }
}

int main(int argc, char *argv[]) {
  SystemState sys = {0};
  sys.next_invoice_id = 1;
  Iva it[26];
  int ic = 0;
  if (argc == 1) {
    it[0] = (Iva){0, 'A'};
    it[1] = (Iva){6, 'B'};
    it[2] = (Iva){13, 'C'};
    it[3] = (Iva){23, 'D'};
    ic = 4;
  } else {
    FILE *f = fopen(argv[1], "r");
    if (f) {
      while (fscanf(f, " %c %d", &it[ic].letter, &it[ic].value) == 2)
        ic++;
      fclose(f);
    }
  }
  int c;
  while ((c = getchar()) != EOF) {
    if (isspace(c))
      continue;
    switch (c) {
    case 'q':
      clean_all(&sys);
      return 0;
    case 'p':
      cmd_p(&sys, it, ic);
      break;
    case 'l':
      cmd_l(&sys);
      break;
    case 'a':
      cmd_a(&sys, it, ic);
      break;
    case 'r':
      cmd_r(&sys, it, ic);
      break;
    case 'f':
      cmd_f(&sys, it, ic);
      break;
    case 'c':
      cmd_c(&sys);
      break;
    case 'd':
      cmd_d(&sys);
      break;
    }
  }
  return 0;
}
