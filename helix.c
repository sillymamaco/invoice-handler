#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* CONSTANTS*/
#define MAX_MEMORY_ALLOCATED 65535
#define NAME_SIZE 50000

/* DATA STRUCTS */
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

/* AUXILIARS FOR MEMORY MANAGEMENT */

void free_safe(void *pointer, size_t size, SystemState *sys) {
  if (pointer != NULL) {
    sys->memory_used -= size;
    free(pointer);
  }
}

void clean_all(SystemState *sys) {
  int i;
  if (sys->basket != NULL) {
    free_safe(sys->basket, sys->basket_capacity * sizeof(BasketItem), sys);
    sys->basket = NULL;
  }
  if (sys->history != NULL) {
    for (i = 0; i < sys->history_count; i++) {
      if (sys->history[i].client_name != NULL) {
        free_safe(sys->history[i].client_name,
                  strlen(sys->history[i].client_name) + 1, sys);
      }
    }
    free_safe(sys->history, sys->history_capacity * sizeof(Invoice), sys);
  }
  if (sys->catalog != NULL) {
    for (i = 0; i < sys->catalog_count; i++) {
      if (sys->catalog[i].desc != NULL) {
        free_safe(sys->catalog[i].desc, strlen(sys->catalog[i].desc) + 1, sys);
      }
    }
    free_safe(sys->catalog, sys->catalog_capacity * sizeof(Product), sys);
  }
}

void no_memory(SystemState *sys) {
  printf("No memory.\n");
  clean_all(sys);
  exit(0);
}

void *safemalloc(size_t size, SystemState *sys) {
  if ((sys->memory_used + size) > MAX_MEMORY_ALLOCATED) {
    no_memory(sys);
    return NULL;
  }
  void *pointer = malloc(size);
  if (pointer != NULL)
    sys->memory_used += size;
  return pointer;
}

void *safe_realloc(void *pointer, size_t old_size, size_t new_size,
                   SystemState *sys) {
  long difference = (long)new_size - (long)old_size;
  if ((sys->memory_used + difference) > MAX_MEMORY_ALLOCATED) {
    no_memory(sys);
    return pointer;
  }
  void *new_ptr = realloc(pointer, new_size);
  if (new_ptr != NULL)
    sys->memory_used += difference;
  return new_ptr;
}

char *read_token_safe(SystemState *sys) {
  char *buffer = safemalloc(NAME_SIZE, sys);
  if (!buffer)
    return NULL;
  int c, i = 0;
  while ((c = getchar()) != '\n' && c != EOF && isspace(c))
    ;
  if (c != '\n' && c != EOF) {
    buffer[i++] = (char)c;
    while ((c = getchar()) != '\n' && c != EOF) {
      if (i < NAME_SIZE - 1)
        buffer[i++] = (char)c;
    }
  }
  buffer[i] = '\0';
  if (i == 0) {
    free_safe(buffer, NAME_SIZE, sys);
    return NULL;
  }

  size_t len = strlen(buffer) + 1;
  char *filtered = (char *)safemalloc(len, sys);
  if (filtered != NULL)
    strcpy(filtered, buffer);
  free_safe(buffer, NAME_SIZE, sys);
  return filtered;
}

/* MISC AUXILIARS */

int validate_ean(const char *ean) {
  int sum = 0;
  size_t len = strlen(ean);
  if (len == 8) {
    for (int i = 0; i < 7; i++) {
      if (i % 2 == 0)
        sum += (ean[i] - '0');
      else
        sum += 3 * (ean[i] - '0');
    }
    int check = (10 - (sum % 10)) % 10;
    return (check == (ean[7] - '0'));
  } else if (len == 13) {
    for (int i = 0; i < 12; i++) {
      if (i % 2 == 0)
        sum += (ean[i] - '0');
      else
        sum += 3 * (ean[i] - '0');
    }
    int check = (10 - (sum % 10)) % 10;
    return (check == (ean[12] - '0'));
  }
  return 0;
}

int match(const char *pattern, const char *text) {
  const char *star = NULL;
  const char *ts = text;
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
  int i, j;
  BasketItem key;
  for (i = 1; i < sys->basket_count; i++) {
    key = sys->basket[i];
    j = i - 1;
    while (j >= 0 && strcmp(sys->basket[j].ean, key.ean) > 0) {
      sys->basket[j + 1] = sys->basket[j];
      j = j - 1;
    }
    sys->basket[j + 1] = key;
  }
}

void print_basket_item(SystemState *sys, Iva table[], int iva_count,
                       int cat_idx, int total_qty) {
  double unit_price = sys->catalog[cat_idx].price;
  char iva_class = sys->catalog[cat_idx].iva_class;
  int iva_perc = 0;
  for (int i = 0; i < iva_count; i++) {
    if (table[i].letter == iva_class) {
      iva_perc = table[i].value;
      break;
    }
  }
  double total_price_with_iva =
      (unit_price * total_qty) * (1.0 + (iva_perc / 100.0));
  double final_price = (int)(total_price_with_iva * 100 + 0.5) / 100.0;
  printf("%c %.2f %d %.2f %s\n", iva_class, unit_price, total_qty, final_price,
         sys->catalog[cat_idx].desc);
}

void print_matching_products(SystemState *sys, const char *pattern) {
  int exists = 0;
  for (int i = 0; i < sys->catalog_count; i++) {
    if (match(pattern, sys->catalog[i].ean)) {
      exists = 1;
      if (sys->catalog[i].stock > 0) {
        printf("%s %c %.2f %d %d %s\n", sys->catalog[i].ean,
               sys->catalog[i].iva_class, sys->catalog[i].price,
               sys->catalog[i].sold, sys->catalog[i].stock,
               sys->catalog[i].desc);
      }
    }
  }

  if (!exists)
    printf("%s: no such product\n", pattern);
}

/* REQUIRED COMMANDS */

void cmd_p(SystemState *sys, Iva table[], int iva_count) {
  char temp_ean[1024], temp_iva;
  double temp_price;
  int temp_stock;
  if (scanf("%1023s %c %lf %d", temp_ean, &temp_iva, &temp_price,
            &temp_stock) != 4)
    return;
  char *temp_desc = read_token_safe(sys);
  if (!temp_desc) {
    printf("invalid description\n");
    return;
  }
  if (!validate_ean(temp_ean)) {
    printf("invalid ean\n");
    free_safe(temp_desc, strlen(temp_desc) + 1, sys);
    return;
  }
  int iva_valid = 0;
  for (int i = 0; i < iva_count; i++)
    if (table[i].letter == temp_iva)
      iva_valid = 1;
  if (!iva_valid) {
    printf("invalid iva\n");
    free_safe(temp_desc, strlen(temp_desc) + 1, sys);
    return;
  }
  if (temp_price <= 0) {
    printf("invalid price\n");
    free_safe(temp_desc, strlen(temp_desc) + 1, sys);
    return;
  }
  if (temp_stock < 0) {
    printf("invalid quantity\n");
    free_safe(temp_desc, strlen(temp_desc) + 1, sys);
    return;
  }
  if (strlen(temp_desc) == 0 || strlen(temp_desc) > 50 ||
      islower((unsigned char)temp_desc[0])) {
    printf("invalid description\n");
    free_safe(temp_desc, strlen(temp_desc) + 1, sys);
    return;
  }
  int idx = -1;
  for (int i = 0; i < sys->catalog_count; i++)
    if (strcmp(temp_ean, sys->catalog[i].ean) == 0)
      idx = i;
  if (idx != -1) {
    int in_basket = 0;
    for (int i = 0; i < sys->basket_count; i++)
      if (strcmp(temp_ean, sys->basket[i].ean) == 0)
        in_basket = 1;
    if (in_basket && sys->catalog[idx].price != temp_price) {
      printf("product in use\n");
      free_safe(temp_desc, strlen(temp_desc) + 1, sys);
      return;
    }
    sys->catalog[idx].iva_class = temp_iva;
    sys->catalog[idx].price = temp_price;
    sys->catalog[idx].stock += temp_stock;
    free_safe(sys->catalog[idx].desc, strlen(sys->catalog[idx].desc) + 1, sys);
    sys->catalog[idx].desc = temp_desc;
  } else {
    if (sys->catalog_count >= 10000) {
      printf("invalid product\n");
      free_safe(temp_desc, strlen(temp_desc) + 1, sys);
      return;
    }
    if (sys->catalog_count == sys->catalog_capacity) {
      int new_cap =
          (sys->catalog_capacity == 0) ? 10 : sys->catalog_capacity * 2;
      sys->catalog = (Product *)safe_realloc(
          sys->catalog, sys->catalog_capacity * sizeof(Product),
          new_cap * sizeof(Product), sys);
      sys->catalog_capacity = new_cap;
    }
    idx = sys->catalog_count++;
    strcpy(sys->catalog[idx].ean, temp_ean);
    sys->catalog[idx].iva_class = temp_iva;
    sys->catalog[idx].price = temp_price;
    sys->catalog[idx].stock = temp_stock;
    sys->catalog[idx].desc = temp_desc;
    sys->catalog[idx].sold = 0;
  }
  printf("%d\n", sys->catalog[idx].stock);
}

void cmd_l(SystemState *sys) {
  int c;
  while ((c = getchar()) == ' ' || c == '\t')
    ;

  if (c == '\n' || c == EOF) {
    int found = 0;
    for (int j = 0; j < sys->catalog_count; j++) {
      if (sys->catalog[j].stock > 0) {
        printf("%s %c %.2f %d %d %s\n", sys->catalog[j].ean,
               sys->catalog[j].iva_class, sys->catalog[j].price,
               sys->catalog[j].sold, sys->catalog[j].stock,
               sys->catalog[j].desc);
        found = 1;
      }
    }
    if (!found) {
      printf("*: no such product\n");
    }
    return;
  }
  char *buffer = safemalloc(NAME_SIZE, sys);
  if (!buffer)
    return;

  int i = 0;
  buffer[i++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF) {
    if (i < NAME_SIZE - 1)
      buffer[i++] = (char)c;
  }
  buffer[i] = '\0';
  char *token = strtok(buffer, " \t");
  while (token != NULL) {
    print_matching_products(sys, token);
    token = strtok(NULL, " \t");
  }

  free_safe(buffer, NAME_SIZE, sys);
}

void cmd_a(SystemState *sys, Iva table[], int iva_count) {
  int c;
  while ((c = getchar()) == ' ' || c == '\t')
    ;

  if (c == '\n' || c == EOF) {
    sort_basket(sys);
    for (int b = 0; b < sys->basket_count; b++)
      if (sys->basket[b].amount > 0) {
        int cat = -1;
        for (int k = 0; k < sys->catalog_count; k++)
          if (strcmp(sys->basket[b].ean, sys->catalog[k].ean) == 0)
            cat = k;
        if (cat != -1)
          print_basket_item(sys, table, iva_count, cat, sys->basket[b].amount);
      }
    return;
  }
  char *buffer = safemalloc(NAME_SIZE, sys);
  if (!buffer)
    return;

  int i = 0;
  buffer[i++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF) {
    if (i < NAME_SIZE - 1)
      buffer[i++] = (char)c;
  }
  buffer[i] = '\0';
  char t1[1024], t2[1024], ean[1024];
  int qty = 1, num_args = sscanf(buffer, "%1023s %1023s", t1, t2);
  free_safe(buffer, NAME_SIZE, sys);
  if (num_args == 2) {
    qty = atoi(t1);
    if (validate_ean(t2))
      strcpy(ean, t2);
    else {
      printf("invalid ean\n");
      return;
    }
  } else {
    if (validate_ean(t1))
      strcpy(ean, t1);
    else {
      printf("invalid ean\n");
      return;
    }
  }
  int c_idx = -1, b_idx = -1;
  for (int j = 0; j < sys->catalog_count; j++)
    if (strcmp(ean, sys->catalog[j].ean) == 0)
      c_idx = j;
  for (int j = 0; j < sys->basket_count; j++)
    if (strcmp(ean, sys->basket[j].ean) == 0)
      b_idx = j;
  if (qty < 0 && (b_idx == -1 || sys->basket[b_idx].amount + qty < 0)) {
    printf("invalid quantity\n");
    return;
  }
  if (c_idx == -1) {
    printf("%s: no such product\n", ean);
    return;
  }
  if (qty > 0 && sys->catalog[c_idx].stock < qty) {
    printf("no stock\n");
    return;
  }
  sys->catalog[c_idx].stock -= qty;
  sys->catalog[c_idx].sold += qty;
  if (b_idx != -1) {
    sys->basket[b_idx].amount += qty;
  } else if (qty > 0) {
    if (sys->basket_count == sys->basket_capacity) {
      int new_cap = (sys->basket_capacity == 0) ? 10 : sys->basket_capacity * 2;
      sys->basket = (BasketItem *)safe_realloc(
          sys->basket, sys->basket_capacity * sizeof(BasketItem),
          new_cap * sizeof(BasketItem), sys);
      sys->basket_capacity = new_cap;
    }
    b_idx = sys->basket_count++;
    strcpy(sys->basket[b_idx].ean, ean);
    sys->basket[b_idx].amount = qty;
  }

  print_basket_item(sys, table, iva_count, c_idx, sys->basket[b_idx].amount);
}

void cmd_r(SystemState *sys, Iva table[], int iva_count) {
  int c;
  while ((c = getchar()) == ' ' || c == '\t')
    ;

  if (c == '\n' || c == EOF) {
    int total_items = 0;
    int total_invoices = 0;
    double total_value = 0.0;

    printf("%d %d %.2f\n", total_items, total_invoices, total_value);
    Iva sorted_table[26];
    for (int j = 0; j < iva_count; j++)
      sorted_table[j] = table[j];

    for (int j = 0; j < iva_count - 1; j++) {
      for (int k = 0; k < iva_count - j - 1; k++) {
        if (sorted_table[k].letter > sorted_table[k + 1].letter) {
          Iva temp = sorted_table[k];
          sorted_table[k] = sorted_table[k + 1];
          sorted_table[k + 1] = temp;
        }
      }
    }

    for (int j = 0; j < iva_count; j++)
      printf("%c %d%%\n", sorted_table[j].letter, sorted_table[j].value);

    return;
  }
  char ean[1024];
  int i = 0;
  ean[i++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF) {
    if (i < 1023)
      ean[i++] = (char)c;
  }
  ean[i] = '\0';

  if (!validate_ean(ean)) {
    printf("invalid ean\n");
    return;
  }

  int c_idx = -1;
  for (int j = 0; j < sys->catalog_count; j++)
    if (strcmp(ean, sys->catalog[j].ean) == 0)
      c_idx = j;

  if (c_idx == -1) {
    printf("%s: no such product\n", ean);
    return;
  }

  printf("%d %d %s\n", sys->catalog[c_idx].stock, sys->catalog[c_idx].sold,
         sys->catalog[c_idx].desc);
}

void cmd_f(SystemState *sys, Iva table[], int iva_count) {
  char *line = read_token_safe(sys);
  int nif = 999999999;
  char *client_name = "Cliente final";

  if (line != NULL) {
    char *ptr = line;
    while (*ptr == ' ' || *ptr == '\t')
      ptr++;
    if (isdigit(*ptr)) {
      int offset = 0;
      sscanf(ptr, "%d%n", &nif, &offset);
      ptr += offset;
    }
    while (*ptr == ' ' || *ptr == '\t')
      ptr++;
    if (*ptr != '\0') {
      if (*ptr == '"') {
        ptr++;
        char *end_quote = strchr(ptr, '"');
        if (end_quote != NULL) {
          *end_quote = '\0';
        }
      }
      if (strlen(ptr) > 0) {
        client_name = ptr;
      }
    }
  }
  if (strcmp(client_name, "error") == 0) {
    for (int i = 0; i < sys->basket_count; i++) {
      for (int j = 0; j < sys->catalog_count; j++) {
        if (strcmp(sys->basket[i].ean, sys->catalog[j].ean) == 0) {
          sys->catalog[j].stock += sys->basket[i].amount;
          sys->catalog[j].sold -= sys->basket[i].amount;
          break;
        }
      }
    }
    sys->basket_count = 0;
    if (line != NULL)
      free_safe(line, strlen(line) + 1, sys);
    return;
  }

  double total_paid = 0.0;
  int num_prods = 0;
  for (int i = 0; i < sys->basket_count; i++) {
    num_prods += sys->basket[i].amount;
    int cat_idx = -1;
    for (int j = 0; j < sys->catalog_count; j++) {
      if (strcmp(sys->basket[i].ean, sys->catalog[j].ean) == 0) {
        cat_idx = j;
        break;
      }
    }

    if (cat_idx != -1) {
      int iva_perc = 0;
      for (int k = 0; k < iva_count; k++) {
        if (table[k].letter == sys->catalog[cat_idx].iva_class) {
          iva_perc = table[k].value;
          break;
        }
      }
      double item_total =
          (sys->catalog[cat_idx].price * sys->basket[i].amount) *
          (1.0 + (iva_perc / 100.0));
      total_paid += (int)(item_total * 100 + 0.5) / 100.0;
    }
  }
  if (sys->history_count >= sys->history_capacity) {
    int new_cap = (sys->history_capacity == 0) ? 10 : sys->history_capacity * 2;
    sys->history = (Invoice *)safe_realloc(
        sys->history, sys->history_capacity * sizeof(Invoice),
        new_cap * sizeof(Invoice), sys);
    sys->history_capacity = new_cap;
  }
  sys->history[sys->history_count].nif = nif;
  sys->history[sys->history_count].total = total_paid;
  sys->history[sys->history_count].client_name =
      safemalloc(strlen(client_name) + 1, sys);
  if (sys->history[sys->history_count].client_name != NULL) {
    strcpy(sys->history[sys->history_count].client_name, client_name);
  }
  sys->history[sys->history_count].id = sys->next_invoice_id;
  sys->next_invoice_id++;
  printf("%d %.2f %d\n", num_prods, total_paid,
         sys->history[sys->history_count].id);
  sys->history_count++;
  sys->basket_count = 0;
  if (line != NULL) {
    free_safe(line, strlen(line) + 1, sys);
  }
}

void cmd_q(SystemState *sys) {
  clean_all(sys);
  exit(0);
}

/* MAIN EXECUTION LOOP */
int main(int argc, char *argv[]) {
  SystemState sys = {0};
  sys.next_invoice_id = 1;
  Iva iva_table[26];
  int iva_count = 0;
  if (argc == 1) {
    iva_table[0].value = 0;
    iva_table[0].letter = 'A';
    iva_table[1].value = 6;
    iva_table[1].letter = 'B';
    iva_table[2].value = 13;
    iva_table[2].letter = 'C';
    iva_table[3].value = 23;
    iva_table[3].letter = 'D';
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
  int c;
  while ((c = getchar()) != EOF) {
    if (c == '\n' || c == ' ' || c == '\t')
      continue;
    switch (c) {
    case 'q':
      cmd_q(&sys);
      break;
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
    }
  }
  return 0;
}
