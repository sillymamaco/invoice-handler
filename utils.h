/**
 * @file utils.h
 * @author ist1117890
 * @brief Utility, hashing, and validation functions.
 */
#ifndef UTILS_H
#define UTILS_H

#include "types.h"
#include <stdint.h>

/**
 * @brief Gets the VAT percentage based on its letter class.
 * @param table Array of Iva structures.
 * @param iva_count Number of VAT elements.
 * @param iva_class The class letter to search.
 * @return The percentage of the given VAT class.
 */
int get_iva_rate(Iva table[], int iva_count, char iva_class);

/**
 * @brief Validates an EAN string.
 * @param ean The EAN to validate.
 * @return 1 if valid, 0 otherwise.
 */
int validate_ean(const char *ean);

/**
 * @brief Matches a string against a pattern containing wildcards.
 * @param pattern The pattern with '*' or '?'.
 * @param text The text to match against.
 * @return 1 if match is successful, 0 otherwise.
 */
int match(const char *pattern, const char *text);

/**
 * @brief Checks if a description string starts with a valid character.
 * @param s The string to check.
 * @return 1 if valid, 0 otherwise.
 */
int is_valid_desc_start(const char *s);

/**
 * @brief Checks if a client name starts with a valid character.
 * @param s The string to check.
 * @return 1 if valid, 0 otherwise.
 */
int is_valid_name_start(const char *s);

/**
 * @brief Rounds a monetary value.
 * @param val Value to round.
 * @return Rounded double.
 */
double round_money(double val);

/**
 * @brief Validates the input for the 'p' command.
 * @param ean Product EAN.
 * @param iva_ok Whether the VAT is valid.
 * @param price Product price.
 * @param stock Product stock.
 * @param desc Product description.
 * @return 1 if valid, 0 otherwise.
 */
int validate_p_input(const char *ean, int iva_ok, double price, int stock,
                     const char *desc);

#endif