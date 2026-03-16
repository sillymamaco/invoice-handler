#include "commands.h"
#include "basket.h"
#include "catalog.h"
#include "invoice.h"
#include "memory.h"
#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

  int idx = find_product_idx(sys, ean, );
  if (idx != -1) {
    for (int i = 0; i < sys->basket_count; i++) {
      if (strcmp(sys->basket[i].ean, ean) == 0 &&
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
    printf("%d\n", sys->catalog[idx].stock);
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
    sys->catalog[idx].iva_class = iva_c;
    sys->catalog[idx].price = price;
    sys->catalog[idx].stock = stock;
    sys->catalog[idx].desc = desc;
    sys->catalog[idx].sold = 0;
  }
  sort_catalog(sys->catalog, 0, sys->catalog_count - 1);
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

    if (!has_wildcard) {
      uint32_t h = get_hash(token);
      int idx = find_product_idx(sys, token, h);
      if (idx != -1 && sys->catalog[idx].stock > 0) {
        print_product(&sys->catalog[idx]);
        found_any = 1;
      }
    } else {
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
    for (int i = 0; i < sys->history_count; i++)
      printf("%d %.2f %s\n", sys->history[i].id, sys->history[i].total,
             sys->history[i].client_name);
  } else {
    if (!is_valid_name_start(name)) {
      printf("invalid name\n");
      if (line)
        free_safe(line, strlen(line) + 1, sys);
      return;
    }
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