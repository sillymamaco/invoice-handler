/*
 * Command 'c' prototypes and parsing utilities.
 * @file c.h
 * @author IST1117890 (Irina Cojocari)
 */

#ifndef C_H
#define C_H

#include "main.h"

/*
 * Read the rest of the current stdin line as a single token.
 * @param sys System state.
 * @return Dynamically allocated string, or NULL for a blank line.
 */
char *read_token_safe(SystemState *sys);

/*
 * Extract a string handling quotes.
 * @param ptr Pointer to input stream.
 * @param name_buf Output buffer.
 * @param name_buf_size Buffer size.
 * @return Non-zero on success.
 */
int extract_quoted_string(const char **ptr, char *name_buf,
                          size_t name_buf_size);

/*
 * Check that a client name starts with a valid character.
 * @param s Name string.
 * @return Non-zero if valid.
 */
int is_valid_name_start(const char *s);

/*
 * Compare two client name strings.
 * @param a First name.
 * @param b Second name.
 * @return Comparison result.
 */
int cmp_names(const char *a, const char *b);

/*
 * Binary-search the client array for a record by name.
 * @param sys System state.
 * @param name Client name.
 * @return Index or -1 if not found.
 */
int find_client_idx(SystemState *sys, const char *name);

/*
 * List invoices for a client or all clients.
 * @param sys System state.
 */
void cmd_c(SystemState *sys);

#endif /* C_H */
