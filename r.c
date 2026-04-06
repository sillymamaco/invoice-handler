/*
 * Command 'r' implementation and IVA utilities.
 * @file r.c
 * @author IST1117890 (Irina Cojocari)
 */

#include "r.h"
#include "l.h"
#include "p.h"

/*
 * Check whether an IVA class letter is defined.
 * @param table IVA rate table.
 * @param letter Class letter.
 * @return Non-zero if present.
 */
int iva_is_present(const Iva table[], char letter) {
  if (letter < 'A' || letter > 'Z')
    return 0;
  return table[letter - 'A'].present;
}

/*
 * Retrieve the tax rate for an IVA class letter.
 * @param table IVA rate table.
 * @param letter Class letter.
 * @return Tax percentage.
 */
int get_iva_rate(const Iva table[], char letter) {
  if (!iva_is_present(table, letter))
    return 0;
  return table[letter - 'A'].value;
}

/*
 * Define or update one slot in the IVA rate table.
 * @param table IVA rate table.
 * @param letter Class letter.
 * @param value Tax percentage.
 */
void iva_set(Iva table[], char letter, int value) {
  if (letter < 'A' || letter > 'Z')
    return;
  table[letter - 'A'].value = value;
  table[letter - 'A'].present = 1;
}

/*
 * Prints the global billing summary and IVA rates.
 * @param sys System state.
 * @param table IVA table.
 */
static void cmd_r_global_summary(SystemState *sys, Iva table[]) {
  printf("%d %d %.2f\n", sys->global_items, sys->next_invoice_id - 1,
         sys->global_sales_cents / 100.0);
  for (int i = 0; i < IVA_TABLE_SIZE; i++) {
    if (table[i].present)
      printf("%c %d%%\n", (char)('A' + i), table[i].value);
  }
}

/*
 * Print billing summary or product stock info.
 * @param sys System state.
 * @param table IVA rate table.
 */
void cmd_r(SystemState *sys, Iva table[]) {
  char buf[MAX_INSTRC_LENGTH] = {0};

  if (!read_line_to_buffer(buf, MAX_INSTRC_LENGTH) && buf[0] == '\0') {
    cmd_r_global_summary(sys, table);
    return;
  }

  char ean[14] = {0};
  int ei = 0;
  for (int i = 0; buf[i] && !isspace((unsigned char)buf[i]); i++) {
    if (ei < 13)
      ean[ei++] = buf[i];
  }
  ean[ei] = '\0';

  if (strlen(ean) == 0 || !validate_ean(ean)) {
    printf("invalid ean\n");
    return;
  }

  int cat_idx = find_product_idx(sys, ean);
  if (cat_idx == -1)
    printf("%s: no such product\n", ean);
  else
    printf("%d %d %s\n", sys->catalog[cat_idx].stock,
           sys->catalog[cat_idx].sold, sys->catalog[cat_idx].desc);
}
