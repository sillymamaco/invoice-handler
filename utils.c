#include "utils.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

int get_iva_rate(Iva table[], int iva_count, char iva_class) {
  for (int i = 0; i < iva_count; i++)
    if (table[i].letter == iva_class)
      return table[i].value;
  return 0;
}

int validate_ean(const char *ean) {
  int sum = 0, len = (int)strlen(ean);
  if (len != EAN_LEN_8 && len != EAN_LEN_13)
    return 0;
  for (int i = 0; i < len - 1; i++) {
    int val = ean[i] - '0';
    sum += (i % 2 == 0) ? val : 3 * val;
  }
  return (((10 - (sum % 10)) % 10) == (ean[len - 1] - '0'));
}

int match(const char *pattern, const char *text) {
  const char *star = NULL, *ts = text;
  while (*text) {
    if (*pattern == '?' || *pattern == *text) {
      pattern++;
      text++;
    } else if (*pattern == '*') {
      star = pattern++;
      ts = text;
    } else if (star) {
      pattern = star + 1;
      text = ++ts;
    } else {
      return 0;
    }
  }
  while (*pattern == '*')
    pattern++;
  return *pattern == '\0';
}

int is_valid_desc_start(const char *s) {
  if (!s || !s[0])
    return 0;
  unsigned char c = (unsigned char)s[0];
  if (c >= 'A' && c <= 'Z')
    return 1;
  if (c >= 0xC0)
    return 1;
  return 0;
}

int is_valid_name_start(const char *s) {
  if (!s || !s[0])
    return 0;
  unsigned char c = (unsigned char)s[0];
  if (isdigit(c))
    return 0;
  if (c >= 'A' && c <= 'Z')
    return 1;
  if (c >= 'a' && c <= 'z')
    return 1;
  if (c >= 0xC0)
    return 1;
  return 0;
}

double round_money(double val) {
  return (long long)(val * PCT_DIV + ROUND_OFFSET) / PCT_DIV;
}

int validate_p_input(const char *ean, int iva_ok, double price, int stock,
                     const char *desc) {
  int desc_valid =
      desc && is_valid_desc_start(desc) && strlen(desc) <= MAX_DESC_LEN;
  if (!validate_ean(ean)) {
    printf("invalid ean\n");
    return 0;
  }
  if (!iva_ok) {
    printf("invalid iva\n");
    return 0;
  }
  if (price <= 0) {
    printf("invalid price\n");
    return 0;
  }
  if (stock < 0) {
    printf("invalid quantity\n");
    return 0;
  }
  if (!desc || !desc_valid) {
    printf("invalid description\n");
    return 0;
  }
  return 1;
}