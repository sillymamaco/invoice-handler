#include <stdio.h>
#include <string.h>
// #include <ctype.h>
#include <stdlib.h>

#define MAX_IVAS 26
#define SIZE_STRING 6
#define EAN_SIZE 14
#define NAME_SIZE 50000

typedef struct {
  char class;
  int percentage;
} Iva;

typedef struct {
  char ean[14];
  int iva;
  float price;
  int stock;
  char desc[51];
  int sold;
  int exists;
} Product;

typedef struct basketNode {
  char ean[14];
  int amount;
  struct basketNode *next;
} BasketItem;

typedef struct {
  int id;
  long nif;
  char *name;
  double total;
} Invoice;

typedef struct invoiceNode {
  Invoice info;
  struct invoiceNode *next;
} InvoiceItem;

/*Ensures cents are rounder correctly */
double round_to_cent(double num) {
  num += 0.005;
  long result;
  result = num * 100;
  result = result / 100.0;
  return result;
}

/*Reads a clients name, whether it has " or not */
int read_string_dynamically() {
  char name[1] = "\0";
  char c;
  char temp[2];
  temp[0] = c;
  temp[1] = '\0';
  if ((c = getchar()) != '\"') {
    strncat(name, temp, 1);
    while ((c = getchar()) != '\"' && c != '\0') {
      char temp[2];
      temp[0] = c;
      temp[1] = '\0';
      strncat(name, temp, 1);
    }
  }
  return 0;
}

/*Does what promises to do */
int load_iva_file(char *filename, Iva table[]) {
  int i = 0;
  char line[SIZE_STRING];
  FILE *file = fopen(filename, "r");
  while (fgets(line, SIZE_STRING, file)) {
    Iva temp;
    temp.class = line[0];
    char temp_str[3];
    temp_str[0] = line[2];
    temp_str[1] = line[3];
    temp_str[2] = '\0';
    temp.percentage = atoi(temp_str);
    table[i] = temp;
    i++;
  }

  fclose(file);
  return i;
}

/*Checks if the last digit is the sum of the even ones and the triple of the
odd ones %10)*/
int validate_ean(char *ean) {
  int len = strlen(ean);
  int sum = 0;
  for (int i = 0; i < len - 1; i++) {
    ean[i] = ean[i] - '0';
    if (ean[i] % 2 == 0)
      sum += ean[i];
    else
      sum += 3 * ean[i];
  }
  return (10 - (sum % 10) % 10) == ean[len - 2];
}

/* gets the iva of the product from the table, no matter the size
(auto or from file)*/
int get_iva(char class, Iva table[], int n_ivas) {
  for (int i = 0; i < n_ivas; i++) {
    if (table[i].class == class)
      return table[i].percentage;
  }
  printf("Invalid IVA.");
  return 1;
}

/*applies the iva to the final price */
double apply_iva(double price, int percentage) {
  int total = price + (percentage * price);
  total = round_to_cent(total);
  return total;
}

/*When the arg has a wildcard, finds all the matches possible for the pattern*/
int match_wildcard(char *pattern, char *text) {
  if (*pattern == '\0')
    return *text == '\0';
  if (*pattern == '*') {
    return match_wildcard(pattern + 1, text) ||
           (*text != '\0' && match_wildcard(pattern, text + 1));
  }
  if (*pattern == '?' && *text != '\0') {
    return match_wildcard(pattern + 1, text + 1);
  }
  if (*pattern == *text) {
    return match_wildcard(pattern + 1, text + 1);
  }
  return 0;
}

// assign ean and add to basket
BasketItem *add_to_basket(BasketItem *head, char *ean, int amt) {
  BasketItem *newItem = (BasketItem *)malloc(sizeof(BasketItem));
  if (newItem == NULL)
    return head;
  newItem->amount = amt;
  strncpy(newItem->ean, ean, EAN_SIZE);
  newItem->next = head->next;
  head->next = newItem;
  return newItem;
}

void clean_basket(BasketItem *head, char *ean, int amt) {
  for (int i = 0; i < amt; i++) {
    BasketItem *temp = head->next;
    head->next = temp->next;
    free(temp);
  }
  free(head);
}

InvoiceItem *create_invoice(InvoiceItem *head, int id, long nif, char *name) {
  Invoice *invoice = (Invoice *)malloc(sizeof(Invoice));
  invoice->id = id;
  invoice->nif = nif;
  strncpy(invoice->name, name, NAME_SIZE);
  head->info = *invoice;
  return head;
}

void sort_invoices(InvoiceItem *head);

/*quits*/
void do_q();

/*adds new product to storage when provided with valid ean, price, amount and
description*/
void do_p(Product prods[], int *amount, Iva table[], int *amtiva,
          BasketItem *basket);

/*lists all items avaliable and their iva, price, amount and total with iva
 and description; can be provided with a pattern with wildcards for the ean to
 be matched with*/
void do_l(Product prods[], int amount);

/*adds x items to the basket. if no x provided assumes one. if no item provided,
lists them*/
void do_a(BasketItem *basket[], Product prods[], int amt);

/*Summarizes total bought, invoices emitted, total earned or the stats
for a specific item*/
void do_r(Product prods[], int amt, Iva table[], int amtivas);

/*does the invoice for the items in the basket. if no nif provided, assume
999999999. if no name provided assume "Cliente final". If name isnt valid,
return an error and dont print the invoice*/
void do_f(InvoiceItem *history[], BasketItem *basket[], Product prods[],
          int amt, int *id_invoice, Iva table[], int amtivas);

/*lists invoices for a customer. if no arg, print by alphabetical order then
chronological, if customer provided, just chronological order*/
void do_c(InvoiceItem *history);

/*delete a product or invoice. doesnt restock the items if it deletes the
invoice. if deletes x items, outputs the amount left and removes from the
available. if nothing left, delete from registred.*/
void do_d(InvoiceItem *history[], Product prods[], int *amt,
          BasketItem *basket);

int main() { return 0; }
