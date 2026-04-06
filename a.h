/*
 * Command 'a' prototypes.
 * @file a.h
 * @author IST1117890 (Irina Cojocari)
 */

#ifndef A_H
#define A_H

#include "main.h"

/*
 * Retrieves the index of an EAN in the current basket.
 * @param sys System state.
 * @param ean EAN to find.
 * @return Basket index or -1 if not found.
 */
int get_basket_idx(SystemState *sys, const char *ean);

/*
 * Add a quantity of a product to the basket.
 * @param sys System state.
 * @param table IVA rate table.
 */
void cmd_a(SystemState *sys, Iva table[]);

#endif /* A_H */
