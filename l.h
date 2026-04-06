/*
 * Command 'l' prototypes and utilities.
 * @file l.h
 * @author IST1117890 (Irina Cojocari)
 */

#ifndef L_H
#define L_H

#include "main.h"

/*
 * Fill buffer from stdin until newline or EOF.
 * @param buf Buffer to fill.
 * @param i Pointer to index.
 * @param truncated Pointer to truncation flag (for big inputs that exceed
 * limit such as client names).
 */
void fill_buffer_from_stdin(char *buf, int *i, int *truncated);

/*
 * Read a full line from stdin into a buffer.
 * @param buf Output buffer.
 * @param limit Buffer size limit.
 * @return Non-zero if line was truncated.
 */
int read_line_to_buffer(char *buf, size_t limit);

/*
 * Test whether text matches a given pattern with wildcards.
 * @param pattern Wildcard pattern.
 * @param text Text to match.
 * @return Non-zero if match is found.
 */
int match(const char *pattern, const char *text);

/*
 * Print one product line.
 * @param p Pointer to the product.
 */
void print_product(const Product *p);

/*
 * List available products.
 * @param sys System state.
 */
void cmd_l(SystemState *sys);

#endif /* L_H */
