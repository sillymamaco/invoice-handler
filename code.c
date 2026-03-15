#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*no magic number round here */
#define MAX_MEMORY_ALLOCATED 65535
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

/* AUXILIARS FOR MEMORY HANDLING */

/*
Does what free does, but updates the amount of data dynamically allocated
Arguments: what to free, how much to free, the State of System struct
*/
void free_safe(void *pointer, size_t size, SystemState *sys) {
  if (pointer) {
    sys->memory_used -= (int)size;
    free(pointer);
  }
}

/*
Ensures there's no dinamically allocated data after the program ends
Arguments: Struct with info about the state of the program
*/
void clean_all(SystemState *sys) {
  if (sys->basket) // clean the basket
    free_safe(sys->basket, sys->basket_capacity * sizeof(BasketItem), sys);
  if (sys->history) { // clean invoice history
    for (int i = 0; i < sys->history_count; i++)
      if (sys->history[i].client_name)
        free_safe(sys->history[i].client_name,
                  strlen(sys->history[i].client_name) + 1, sys);
    free_safe(sys->history, sys->history_capacity * sizeof(Invoice), sys);
  }
  if (sys->catalog) { // clean the inventory
    for (int i = 0; i < sys->catalog_count; i++)
      if (sys->catalog[i].desc)
        free_safe(sys->catalog[i].desc, strlen(sys->catalog[i].desc) + 1, sys);
    free_safe(sys->catalog, sys->catalog_capacity * sizeof(Product), sys);
  }
}

/*
SOS function for when the memory limit is surpassed
Arguments: Struct with program info
 */
void no_memory(SystemState *sys) {
  printf("No memory.\n");
  clean_all(sys);
  exit(0);
}

/*
malloc, but ensures theres enough space to allocate before doing so
Arguments: size to allocate, info about the state of the program
*/
void *safemalloc(size_t size, SystemState *sys) {
  if ((sys->memory_used + (int)size) > MAX_MEMORY_ALLOCATED)
    no_memory(sys);
  void *p = malloc(size);
  if (p)
    sys->memory_used += (int)size;
  return p;
}

/*
realloc, but ensures theres enough memory to allocate, if the new size
is bigger
Arguments: pointer to reallocate, old memory size, new memory size,
info about the state of the program
*/
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

/*
Auxiliar that reads and sanitizes input
Arguments: info about the state of the program
*/
char *read_token_safe(SystemState *sys) {
  char buf[MAX_INSTRC_LENGTH];
  int c, i = 0;
  /*Skip leading spaces*/
  while ((c = getchar()) != '\n' && c != EOF && isspace(c))
    ;
  if (c != '\n' && c != EOF) {
    buf[i++] = (char)c;
    while ((c = getchar()) != '\n' && c != EOF)
      if (i < MAX_INSTRC_LENGTH && c != '\r') // ignores return char
        buf[i++] = (char)c;
  }
  /*Trims spaces at the end*/
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

/*
Takes an ean code as an argument and returns the index of that
product in the catalog (auxiliar)
Arguments: Info about the state of the program, ean of the product
*/
int find_product_idx(SystemState *sys, const char *ean) {
  for (int i = 0; i < sys->catalog_count; i++)
    if (strcmp(sys->catalog[i].ean, ean) == 0)
      return i;
  return -1;
}

/*
For a given iva class (char), gets the respective iva rate/percentage
Arguments: table with iva classes and their values, amount of iva classes,
class we're looking for
*/
int get_iva_rate(Iva table[], int iva_count, char iva_class) {
  for (int i = 0; i < iva_count; i++)
    if (table[i].letter == iva_class)
      return table[i].value;
  return 0;
}

/*
Auxiliar to validate ean codes (takes them as strings)
Arguments: ean code to validate
*/
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

/*
Helper to match ean codes to patterns that mix * and ? wildcards
and numbers
Arguments: pattern (mix of numbers and * and ?) and a ean code
*/
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

/*
Applies insertion sort(stable and fast) to the basket, sorting products
by ean code.
Arguments: info about the state of the system
*/
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

/*
Prints all the attributes of a product in the required order
Arguments: info about the state of the program, table with iva classes
and their values, amount of iva values, index of product in the catalog.
amount of product in stock
*/
void print_basket_item(SystemState *sys, Iva table[], int iva_count,
                       int cat_idx, int qty) {
  double price = sys->catalog[cat_idx].price;
  int iva = get_iva_rate(table, iva_count, sys->catalog[cat_idx].iva_class);
  double total = (price * qty) * (1.0 + (iva / 100.0));
  printf("%c %.2f %d %.2f %s\n", sys->catalog[cat_idx].iva_class, price, qty,
         (long)((total * 100) + 0.5) / 100.0, sys->catalog[cat_idx].desc);
}

/*
Handles diacriticals in the beggining of descriptions/names
Arguments: first character of a word
*/
int is_valid_first_char(const unsigned char *str) {
  if (!str || !str[0])
    return 0;
  if (str[0] >= 'A' && str[0] <= 'Z')
    return 1;
  if (str[0] == (unsigned char)0xC3 && str[1] == (unsigned char)0x81)
    return 1;
  return 0;
}

/* REQUIRED FUNCTIONS */

/*
Command p: adds or updates products in the catalog/stock
Arguments: info about the state of the program, iva table with
classes and their values, amount of iva values
*/
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
  int desc_valid =
      desc && is_valid_first_char((unsigned char *)desc) && strlen(desc) <= 50;
  if (!validate_ean(ean) || !iva_ok || price <= 0 || stock < 0 || !desc ||
      !desc_valid) { // error handling
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
    // check if the product is in the basket
    for (int i = 0; i < sys->basket_count; i++)
      if (strcmp(sys->basket[i].ean, ean) == 0 &&
          sys->catalog[idx].price != price) {
        printf("product in use\n");
        free_safe(desc, strlen(desc) + 1, sys);
        return;
      }
    // update product info
    sys->catalog[idx].iva_class = iva_c;
    sys->catalog[idx].price = price;
    sys->catalog[idx].stock += stock;
    free_safe(sys->catalog[idx].desc, strlen(sys->catalog[idx].desc) + 1, sys);
    sys->catalog[idx].desc = desc;
  } else {
    if (sys->catalog_count == sys->catalog_capacity) {
      int new_capacity = sys->catalog_capacity ? sys->catalog_capacity * 2 : 10;
      sys->catalog = // if needed, expand the memory allocated for the catalog
          safe_realloc(sys->catalog, sys->catalog_capacity * sizeof(Product),
                       new_capacity * sizeof(Product), sys);
      sys->catalog_capacity = new_capacity;
    } // create the new product
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

/*
Command l: lists all products, or info about the ones that match the pattern
Arguments: info about the state of the program
*/
void cmd_l(SystemState *sys) {
  int c;
  while ((c = getchar()) == ' ' || c == '\t' || c == '\r')
    ;
  if (c == '\n' || c == EOF) { // if no pattern provided
    int found = 0;
    for (int i = 0; i < sys->catalog_count; i++)
      if (sys->catalog[i].stock > 0) {
        printf("%s %c %.2f %d %d %s\n", sys->catalog[i].ean,
               sys->catalog[i].iva_class, sys->catalog[i].price,
               sys->catalog[i].sold, sys->catalog[i].stock,
               sys->catalog[i].desc);
        found = 1;
      }
    if (!found)
      printf("*: no such product\n");
    return;
  }
  char buf[MAX_INSTRC_LENGTH];
  int i = 0;
  buf[i++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF)
    if (i < MAX_INSTRC_LENGTH && c != '\r')
      buf[i++] = (char)c;
  buf[i] = '\0';
  char *token = strtok(buf, " \t\r");
  while (token) {
    int exists = 0;
    for (int j = 0; j < sys->catalog_count; j++)
      if (match(token, sys->catalog[j].ean)) {
        if (sys->catalog[j].stock > 0)
          printf("%s %c %.2f %d %d %s\n", sys->catalog[j].ean,
                 sys->catalog[j].iva_class, sys->catalog[j].price,
                 sys->catalog[j].sold, sys->catalog[j].stock,
                 sys->catalog[j].desc);
        exists = 1;
      }
    if (!exists)
      printf("%s: no such product\n", token);
    token = strtok(NULL, " \t\r");
  }
}

/*
Command a: adds product to basket, whether its new or increase quantity
Arguments: info about the state of the program, table with iva classes and their
values , amount of iva clsses
*/
void cmd_a(SystemState *sys, Iva table[], int iva_count) {
  int c;
  while ((c = getchar()) == ' ' || c == '\t' || c == '\r')
    ;
  if (c == '\n' || c == EOF) {
    sort_basket(sys);
    for (int i = 0; i < sys->basket_count; i++) {
      int cat_idx = find_product_idx(sys, sys->basket[i].ean);
      if (cat_idx != -1)
        print_basket_item(sys, table, iva_count, cat_idx,
                          sys->basket[i].amount);
    }
    return;
  }
  char buf[BUFFER_LIMIT];
  int i = 0;
  buf[i++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF)
    if (i < BUFFER_LIMIT && c != '\r')
      buf[i++] = (char)c;
  buf[i] = '\0';
  char s1[BUFFER_LIMIT], s2[BUFFER_LIMIT], ean[14];
  int qty = 1;
  int n = sscanf(buf, "%s %s", s1, s2);
  if (n == 2) { // if 2 args
    qty = atoi(s1);
    strcpy(ean, s2);
  } else // if 1 arg
    strcpy(ean, s1);
  int cat_idx = find_product_idx(sys, ean), basket_idx = -1;
  for (int j = 0; j < sys->basket_count; j++)
    if (strcmp(sys->basket[j].ean, ean) == 0)
      basket_idx = j;
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
  // update product stats
  sys->catalog[cat_idx].stock -= qty;
  sys->catalog[cat_idx].sold += qty;
  if (basket_idx != -1)
    sys->basket[basket_idx].amount += qty;
  else {
    if (sys->basket_count == sys->basket_capacity) {
      // if needed, expand the dynamic array where basket is ekept
      int new_capacity = sys->basket_capacity ? sys->basket_capacity * 2 : 10;
      sys->basket =
          safe_realloc(sys->basket, sys->basket_capacity * sizeof(BasketItem),
                       new_capacity * sizeof(BasketItem), sys);
      sys->basket_capacity = new_capacity;
    }
    basket_idx = sys->basket_count++;
    strcpy(sys->basket[basket_idx].ean, ean);
    sys->basket[basket_idx].amount = qty;
  }
  // print the result item
  print_basket_item(sys, table, iva_count, cat_idx,
                    sys->basket[basket_idx].amount);
}

/*
Command f: finalizes the sale and generates the invoice
Arguments: info about the state of the program, table with iva classes and
their values, amount of iva classes
*/
void cmd_f(SystemState *sys, Iva table[], int iva_count) {
  char *line = read_token_safe(sys);
  int nif = 999999999; // default values
  char name[1024] = "Cliente final";
  if (line) {
    char *ptr = line;
    while (*ptr && isspace(*ptr))
      ptr++;
    if (isdigit(*ptr)) { // if line starts with nif
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
  if (strcmp(name, "error") == 0) { // if name not found,
                                    // remove items from basket
    for (int i = 0; i < sys->basket_count; i++) {
      int cat_idx = find_product_idx(sys, sys->basket[i].ean);
      if (cat_idx != -1) {
        sys->catalog[cat_idx].stock += sys->basket[i].amount;
        sys->catalog[cat_idx].sold -= sys->basket[i].amount;
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
  int items = 0; // check the total price for the basket
  for (int i = 0; i < sys->basket_count; i++) {
    int cat_idx = find_product_idx(sys, sys->basket[i].ean);
    if (cat_idx != -1 && sys->basket[i].amount > 0) {
      items += sys->basket[i].amount;
      int iva = get_iva_rate(table, iva_count, sys->catalog[cat_idx].iva_class);
      double sub = (sys->catalog[cat_idx].price * sys->basket[i].amount) *
                   (1.0 + (iva / 100.0)); // round simetrically
      total += (long)((sub * 100) + 0.5) / 100.0;
    }
  } // if needed expand the dinamic array that keeps track of invoices
  if (sys->history_count == sys->history_capacity) {
    int new_capacity = sys->history_capacity ? sys->history_capacity * 2 : 10;
    sys->history =
        safe_realloc(sys->history, sys->history_capacity * sizeof(Invoice),
                     new_capacity * sizeof(Invoice), sys);
    sys->history_capacity = new_capacity;
  } // fill in invoice info
  int history_idx = sys->history_count++;
  sys->history[history_idx].nif = nif;
  sys->history[history_idx].total = total;
  sys->history[history_idx].id = sys->next_invoice_id++;
  sys->history[history_idx].num_items = items;
  sys->history[history_idx].client_name = safemalloc(strlen(name) + 1, sys);
  strcpy(sys->history[history_idx].client_name, name);
  printf("%d %.2f %d\n", items, total, sys->history[history_idx].id);
  sys->basket_count = 0;
  if (line)
    free_safe(line, strlen(line) + 1, sys);
}

/*
Command r: reports summaries of the system or individual products
Arguments: info about state of the program, tablet with iva classes
and their values, amount of iva classes
*/
void cmd_r(SystemState *sys, Iva table[], int iva_count) {
  int c; // ignore space chars
  while ((c = getchar()) == ' ' || c == '\t' || c == '\r')
    ;
  if (c == '\n' || c == EOF) { // if no args
    int total_items = 0;
    double total_sales = 0;
    for (int i = 0; i < sys->history_count; i++) {
      total_items += sys->history[i].num_items;
      total_sales += sys->history[i].total;
    }
    printf("%d %d %.2f\n", total_items, sys->next_invoice_id - 1, total_sales);

    Iva sorted_table[26];
    memcpy(sorted_table, table, iva_count * sizeof(Iva));

    // sort ivas using insertion sort (memcopy bc its temporary, just for
    // sorting)
    for (int i = 1; i < iva_count; i++) {
      Iva key = sorted_table[i];
      int j = i - 1;
      while (j >= 0 && sorted_table[j].letter > key.letter) {
        sorted_table[j + 1] = sorted_table[j];
        j = j - 1;
      }
      sorted_table[j + 1] = key;
    }

    for (int i = 0; i < iva_count; i++)
      printf("%c %d%%\n", sorted_table[i].letter, sorted_table[i].value);
    return;
  }
  char ean[14]; // if arg, give product info if ean is right
  ean[0] = (char)c;
  scanf("%s", ean + 1);
  int cat_idx = find_product_idx(sys, ean);
  if (cat_idx == -1)
    printf("%s: no such product\n", ean);
  else
    printf("%d %d %s\n", sys->catalog[cat_idx].stock,
           sys->catalog[cat_idx].sold, sys->catalog[cat_idx].desc);
}

/*
Command c: lists invoices based on client name or all if empty
Arguments: info about the state of the system
*/
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
        char *end = strrchr(ptr, '"');
        if (end)
          *end = '\0';
        strcpy(name, ptr);
      } else {
        strcpy(name, ptr);
      }
    }
  }

  if (strlen(name) == 0) { // if no client name provided
    Invoice **sorted_refs =
        safemalloc(sys->history_count * sizeof(Invoice *), sys);
    for (int i = 0; i < sys->history_count; i++)
      sorted_refs[i] = &sys->history[i];
    // insertion sort on the invoices: stable and efficient
    for (int i = 1; i < sys->history_count; i++) {
      Invoice *key = sorted_refs[i];
      int j = i - 1;
      while (j >= 0) {
        int cmp = strcmp(sorted_refs[j]->client_name, key->client_name);
        if (cmp > 0 || (cmp == 0 && sorted_refs[j]->id > key->id)) {
          sorted_refs[j + 1] = sorted_refs[j];
          j--;
        } else {
          break;
        }
      }
      sorted_refs[j + 1] = key;
    }
    for (int i = 0; i < sys->history_count; i++)
      printf("%d %.2f %s\n", sorted_refs[i]->id, sorted_refs[i]->total,
             sorted_refs[i]->client_name);
    free_safe(sorted_refs, sys->history_count * sizeof(Invoice *), sys);
  } else { // if argument is provided
    int found = 0;
    for (int i = 0; i < sys->history_count; i++)
      if (strcmp(sys->history[i].client_name, name) == 0) {
        printf("%d %.2f %s\n", sys->history[i].id, sys->history[i].total,
               sys->history[i].client_name);
        found = 1;
      }
    if (!found)
      printf("%s: no such client\n", name);
  }
  if (line)
    free_safe(line, strlen(line) + 1, sys);
}

/*
Command d: deletes an invoice by ID or reduces stock of a product
Arguments: info about the state of the program
*/
void cmd_d(SystemState *sys) {
  int c;
  while ((c = getchar()) == ' ' || c == '\t' || c == '\r')
    ;
  if (c == '\n' || c == EOF)
    return;
  char buf[MAX_INSTRC_LENGTH];
  int i = 0;
  buf[i++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF)
    if (i < MAX_INSTRC_LENGTH && c != '\r')
      buf[i++] = (char)c;
  buf[i] = '\0';
  char arg1[BUFFER_LIMIT], arg2[BUFFER_LIMIT];
  int num_args = sscanf(buf, "%1023s %1023s", arg1, arg2);
  if (num_args == 1) { // if one arg, is the id of the invoice
    int invoice_id = atoi(arg1), history_idx = -1;
    for (int j = 0; j < sys->history_count; j++)
      if (sys->history[j].id == invoice_id) {
        history_idx = j;
        break;
      }
    if (history_idx == -1)
      printf("%d: no such invoice\n", invoice_id);
    else { // prints the invoice before deleting it
      printf("%.2f %d %s\n", sys->history[history_idx].total,
             sys->history[history_idx].nif,
             sys->history[history_idx].client_name);
      free_safe(sys->history[history_idx].client_name,
                strlen(sys->history[history_idx].client_name) + 1, sys);
      for (int j = history_idx; j < sys->history_count - 1; j++)
        sys->history[j] = sys->history[j + 1];
      sys->history_count--;
    }
  } else if (num_args == 2) { // if two args, they are ean and amount
    int reduction_qty = atoi(arg2);
    int cat_idx = find_product_idx(sys, arg1);
    if (cat_idx == -1)
      printf("%s: no such product\n", arg1);
    else {
      int product_in_basket = 0;
      for (int j = 0; j < sys->basket_count; j++)
        if (strcmp(sys->basket[j].ean, arg1) == 0)
          product_in_basket = 1;
      if (product_in_basket)
        printf("product in use\n");
      else if (reduction_qty <= 0 ||
               reduction_qty > sys->catalog[cat_idx].stock)
        printf("invalid quantity\n");
      else { // if no more stock, delete product and print it
        sys->catalog[cat_idx].stock -= reduction_qty;
        if (sys->catalog[cat_idx].stock == 0) {
          printf("0 %s\n", sys->catalog[cat_idx].desc);
          free_safe(sys->catalog[cat_idx].desc,
                    strlen(sys->catalog[cat_idx].desc) + 1, sys);
          for (int j = cat_idx; j < sys->catalog_count - 1; j++)
            sys->catalog[j] = sys->catalog[j + 1];
          sys->catalog_count--;
        } else
          printf("%d %s\n", sys->catalog[cat_idx].stock,
                 sys->catalog[cat_idx].desc);
      }
    }
  }
}

/* INITIALIZATION OF VARIABLES */
int main(int argc, char *argv[]) {
  SystemState sys = {0};
  sys.next_invoice_id = 1;
  Iva iva_table[26];
  int iva_count = 0;
  if (argc == 1) { // if no ivas provided use the default values
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

  /* MAIN LOOP */
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
  return 0;
}
