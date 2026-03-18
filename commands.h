/**
 * @file commands.h
 * @brief Public prototypes for the billing system command handlers.
 *
 * @details Each handler reads its arguments directly from @c stdin and writes
 * its output to @c stdout. The IVA table is passed as a fixed array of
 * ::IVA_TABLE_SIZE entries indexed by @c (letter - 'A').
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include "common.h"

/**
 * @brief Insert or update a product in the catalog (@c p command).
 * @param sys   System state.
 * @param table IVA rate table.
 */
void cmd_p(SystemState *sys, Iva table[]);

/**
 * @brief List available products (@c l command).
 * @param sys System state.
 */
void cmd_l(SystemState *sys);

/**
 * @brief Add a quantity of a product to the basket (@c a command).
 * @param sys   System state.
 * @param table IVA rate table (needed to print line totals).
 */
void cmd_a(SystemState *sys, Iva table[]);

/**
 * @brief Finalise the basket into an invoice (@c f command).
 * @param sys   System state.
 * @param table IVA rate table.
 */
void cmd_f(SystemState *sys, Iva table[]);

/**
 * @brief Print billing summary or product stock info (@c r command).
 * @param sys   System state.
 * @param table IVA rate table (printed by bare @c r).
 */
void cmd_r(SystemState *sys, Iva table[]);

/**
 * @brief List invoices for a client or all clients (@c c command).
 * @param sys System state.
 */
void cmd_c(SystemState *sys);

/**
 * @brief Delete an invoice or reduce product stock (@c d command).
 * @param sys System state.
 */
void cmd_d(SystemState *sys);

#endif /* COMMANDS_H */