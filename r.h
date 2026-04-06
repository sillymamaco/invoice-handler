/*
 * Command 'r' prototypes and IVA utilities.
 * @file r.h
 * @author IST1117890 (Irina Cojocari)
 */

#ifndef R_H
#define R_H

#include "main.h"

/*
 * Check whether an IVA class letter is defined.
 * @param table IVA rate table.
 * @param letter Class letter.
 * @return Non-zero if present.
 */
int iva_is_present(const Iva table[], char letter);

/*
 * Retrieve the tax rate for an IVA class letter.
 * @param table IVA rate table.
 * @param letter Class letter.
 * @return Tax percentage.
 */
int get_iva_rate(const Iva table[], char letter);

/*
 * Define or update one slot in the IVA rate table.
 * @param table IVA rate table.
 * @param letter Class letter.
 * @param value Tax percentage.
 */
void iva_set(Iva table[], char letter, int value);

/*
 * Print billing summary or product stock info.
 * @param sys System state.
 * @param table IVA rate table.
 */
void cmd_r(SystemState *sys, Iva table[]);

#endif /* R_H */
