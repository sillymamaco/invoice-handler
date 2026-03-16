/**
 * @file commands.h
 * @brief Definitions for command executors.
 * @author ist1117890
 */
#ifndef COMMANDS_H
#define COMMANDS_H

#include "types.h"

/**
 * @brief Executes the 'p' command (Add/Update Product).
 * @param sys Pointer to system state.
 * @param table VAT configuration table.
 * @param iva_count Total entries in VAT table.
 */
void cmd_p(SystemState *sys, Iva table[], int iva_count);

/**
 * @brief Executes the 'l' command (List Products).
 * @param sys Pointer to system state.
 */
void cmd_l(SystemState *sys);

/**
 * @brief Executes the 'a' command (Add to Basket).
 * @param sys Pointer to system state.
 * @param table VAT configuration table.
 * @param iva_count Total entries in VAT table.
 */
void cmd_a(SystemState *sys, Iva table[], int iva_count);

/**
 * @brief Executes the 'f' command (Finalize Invoice).
 * @param sys Pointer to system state.
 * @param table VAT configuration table.
 * @param iva_count Total entries in VAT table.
 */
void cmd_f(SystemState *sys, Iva table[], int iva_count);

/**
 * @brief Executes the 'r' command (Sales status or Single Product status).
 * @param sys Pointer to system state.
 * @param table VAT configuration table.
 * @param iva_count Total entries in VAT table.
 */
void cmd_r(SystemState *sys, Iva table[], int iva_count);

/**
 * @brief Executes the 'c' command (Client Invoice Search).
 * @param sys Pointer to system state.
 */
void cmd_c(SystemState *sys);

/**
 * @brief Executes the 'd' command (Delete Invoice or Reduce Stock).
 * @param sys Pointer to system state.
 */
void cmd_d(SystemState *sys);

#endif