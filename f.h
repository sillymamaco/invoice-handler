/*
 * Command 'f' prototypes.
 * @file f.h
 * @author IST1117890 (Irina Cojocari)
 */

#ifndef F_H
#define F_H

#include "main.h"

/*
 * Return all basket quantities to stock and reset basket_count to 0.
 * @param sys System state.
 */
void cancel_basket(SystemState *sys);

/*
 * Return the index of a ClientRecord, inserting a new one if needed.
 * @param sys System state.
 * @param name Client name.
 * @param nif Client NIF.
 * @return Index of the client.
 */
int get_or_create_client(SystemState *sys, const char *name, int nif);

/*
 * Finalise the basket into an invoice.
 * @param sys System state.
 * @param table IVA rate table.
 */
void cmd_f(SystemState *sys, Iva table[]);

#endif /* F_H */
