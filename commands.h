/**
 * @file commands.h
 * @author IST1117890 (Irina Cojocari)
 * @brief Public prototypes for the billing system command handlers.
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include "common.h"

/**
 * @brief Insert or update a product in the catalog.
 * @param sys System state.
 * @param table IVA rate table.
 */
void cmd_p(SystemState *sys, Iva table[]);

/**
 * @brief List available products.
 * @param sys System state.
 */
void cmd_l(SystemState *sys);

/**
 * @brief Add a quantity of a product to the basket.
 * @param sys System state.
 * @param table IVA rate table.
 */
void cmd_a(SystemState *sys, Iva table[]);

/**
 * @brief Finalise the basket into an invoice.
 * @param sys System state.
 * @param table IVA rate table.
 */
void cmd_f(SystemState *sys, Iva table[]);

/**
 * @brief Print billing summary or product stock info.
 * @param sys System state.
 * @param table IVA rate table.
 */
void cmd_r(SystemState *sys, Iva table[]);

/**
 * @brief List invoices for a client or all clients.
 * @param sys System state.
 */
void cmd_c(SystemState *sys);

/**
 * @brief Delete an invoice or reduce product stock.
 * @param sys System state.
 */
void cmd_d(SystemState *sys);

#endif /* COMMANDS_H */