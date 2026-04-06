/*
 * Command 'l' implementation and utilities.
 * @file l.c
 * @author IST1117890 (Irina Cojocari)
 */

#include "l.h"
#include "p.h"

/*
 * Continue filling buffer from stdin until newline or EOF.
 * @param buf Buffer to fill.
 * @param i Pointer to index.
 * @param truncated Pointer to truncation flag.
 */
void fill_buffer_from_stdin(char *buf, int *i, int *truncated) {
  int c;
  while ((c = getchar()) != '\n' && c != EOF) {
    if (*i < MAX_INSTRC_LENGTH - 1)
      buf[(*i)++] = (char)c;
    else
      *truncated = 1;
  }
}

/*
 * Skips whitespace and returns the first non-space character.
 * @return The first non-space character encountered.
 */
static int skip_leading_spaces(void) {
  int c;
  while ((c = getchar()) == ' ' || c == '\t')
    ;
  return c;
}

/*
 * Discards all remaining characters on the current stdin line.
 */
static void drain_stdin_line(void) {
  int c;
  while ((c = getchar()) != '\n' && c != EOF)
    ;
}

/*
 * Read a full line from stdin into a buffer.
 * @param buf Output buffer.
 * @param limit Buffer size limit.
 * @return Non-zero if line was truncated.
 */
int read_line_to_buffer(char *buf, size_t limit) {
  int c = skip_leading_spaces();
  if (c == '\n' || c == EOF)
    return 0;

  int i = 0, truncated = 0;
  buf[i++] = (char)c;
  while ((c = getchar()) != '\n' && c != EOF) {
    if (i < (int)limit - 1)
      buf[i++] = (char)c;
    else
      truncated = 1;
  }

  if (truncated)
    drain_stdin_line();
  buf[i] = '\0';
  return truncated;
}

/*
 * Test whether text matches a shell-style pattern.
 * @param pattern Wildcard pattern.
 * @param text Text to match.
 * @return Non-zero if match is found.
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
      /* if there's starts to "spend" while it doesnt match doesnt match */
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

/*
 * Print one product line.
 * @param p Pointer to the product.
 */
void print_product(const Product *p) {
  printf("%s %c %.2f %d %d %s\n", p->ean, p->iva_class, p->price, p->sold,
         p->stock, p->desc);
}

/*
 * Swaps two product pointers.
 * @param a First pointer.
 * @param b Second pointer.
 */
static void swap_ordered(Product **a, Product **b) {
  Product *t = *a;
  *a = *b;
  *b = t;
}

/*
 * Partition function for quicksort by insertion order.
 * @param arr Array of product pointers.
 * @param lo Lower index.
 * @param hi Higher index.
 * @return Partition index.
 */
static int partition_ordered(Product **arr, int lo, int hi) {
  int pivot = arr[hi]->insert_order, i = lo;
  for (int j = lo; j < hi; j++) {
    if (arr[j]->insert_order < pivot) {
      swap_ordered(&arr[i], &arr[j]);
      i++;
    }
  }
  swap_ordered(&arr[i], &arr[hi]);
  return i;
}

/*
 * Quicksort implementation for insertion order.
 * @param arr Array of product pointers.
 * @param lo Lower index.
 * @param hi Higher index.
 */
static void quick_sort_ordered(Product **arr, int lo, int hi) {
  if (lo < hi) {
    int pi = partition_ordered(arr, lo, hi);
    quick_sort_ordered(arr, lo, pi - 1);
    quick_sort_ordered(arr, pi + 1, hi);
  }
}

/*
 * Allocate and return a Product* array sorted by insert_order.
 * @param sys System state.
 * @return Sorted array of Product pointers.
 */
static Product **build_ordered(SystemState *sys) {
  if (sys->catalog_count == 0)
    return NULL;
  Product **ordered = safemalloc(sys->catalog_count * sizeof(Product *), sys);
  for (int k = 0; k < sys->catalog_count; k++)
    ordered[k] = &sys->catalog[k];
  if (sys->catalog_count > 1)
    quick_sort_ordered(ordered, 0, sys->catalog_count - 1);
  return ordered;
}

/*
 * Print in-stock products matching a token.
 * @param sys System state.
 * @param ordered Insertion-order pointer array.
 * @param token EAN string or wildcard pattern.
 * @return Non-zero if anything was printed.
 */
static int print_l_token(SystemState *sys, Product **ordered,
                         const char *token) {
  int found = 0;
  if (strchr(token, '*') || strchr(token, '?')) {
    for (int j = 0; j < sys->catalog_count; j++) {
      if (match(token, ordered[j]->ean) && ordered[j]->stock > 0) {
        print_product(ordered[j]);
        found = 1;
      }
    }
  } else {
    int idx = find_product_idx(sys, token);
    if (idx != -1 && sys->catalog[idx].stock > 0) {
      print_product(&sys->catalog[idx]);
      found = 1;
    }
  }
  return found;
}

/*
 * Print all in-stock products in insertion order.
 * @param sys System state.
 * @param ordered Insertion-order pointer array.
 */
static void cmd_l_all(SystemState *sys, Product **ordered) {
  int found = 0;
  for (int i = 0; i < sys->catalog_count; i++) {
    if (ordered[i]->stock > 0) {
      print_product(ordered[i]);
      found = 1;
    }
  }
  if (!found)
    printf("*: no such product\n");
}

/*
 * Print products for each whitespace-separated token.
 * @param sys System state.
 * @param ordered Insertion-order pointer array.
 * @param buf Mutable token string.
 */
static void cmd_l_tokens(SystemState *sys, Product **ordered, char *buf) {
  char *token = strtok(buf, " \t\n");
  while (token) {
    if (!print_l_token(sys, ordered, token))
      printf("%s: no such product\n", token);
    token = strtok(NULL, " \t\n");
  }
}

/*
 * List available products.
 * @param sys System state.
 */
void cmd_l(SystemState *sys) {
  char buf[MAX_INSTRC_LENGTH] = {0};
  Product **ordered = build_ordered(sys);

  if (!read_line_to_buffer(buf, MAX_INSTRC_LENGTH) && buf[0] == '\0') {
    cmd_l_all(sys, ordered);
  } else {
    cmd_l_tokens(sys, ordered, buf);
  }

  if (ordered)
    free_safe(ordered, sys->catalog_count * sizeof(Product *), sys);
}
