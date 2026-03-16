/**
 * @file invoice.h
 * @author ist1117890
 * @brief Invoice management, creation, and deletion.
 */
#ifndef INVOICE_H
#define INVOICE_H

#include "types.h"

/**
 * @brief Parses client NIF and Name from the given command line string.
 * @param line String containing the arguments.
 * @param nif Pointer to store the extracted NIF.
 * @param name Pointer to store the extracted client name string.
 */
void parse_invoice_client(char *line, int *nif, char **name);

/**
 * @brief Finalizes an invoice, computing total, adding to history.
 * @param sys Pointer to the system state.
 * @param table Array of VAT classes.
 * @param iva_count Total VAT count.
 * @param nif Client NIF.
 * @param name Client Name.
 */
void finalize_invoice(SystemState *sys, Iva table[], int iva_count, int nif,
                      const char *name);

/**
 * @brief Deletes a specific invoice by its ID.
 * @param sys Pointer to the system state.
 * @param inv_id ID of the invoice to delete.
 */
void cmd_d_delete_inv(SystemState *sys, int inv_id);

#endif