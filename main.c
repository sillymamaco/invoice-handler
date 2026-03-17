/**
 * @file main.c
 * @brief Main entry point containing initialization and command dispatch loop.
 */

#include "common.h"
#include "memory.h"
#include "commands.h"

int main(int argc, char *argv[]) {
  SystemState sys = {0};
  sys.next_invoice_id = 1;
  Iva iva_table[26];
  int iva_count = 0;

  if (argc == 1) {
    iva_table[0] = (Iva){0, 'A'};
    iva_table[1] = (Iva){6, 'B'};
    iva_table[2] = (Iva){13, 'C'};
    iva_table[3] = (Iva){23, 'D'};
    iva_count = 4;
  } else {
    FILE *f = fopen(argv[1], "r");
    if (f) {
      while (iva_count < 26 && fscanf(f, " %c %d", &iva_table[iva_count].letter,
                    &iva_table[iva_count].value) == 2)
        iva_count++;
      fclose(f);
    }
  }

  int command;
  while ((command = getchar()) != EOF) {
    if (isspace(command))
      continue;
    switch (command) {
    case 'q':
      clean_all(&sys);
      return 0;
    case 'p':
      cmd_p(&sys, iva_table, iva_count);
      break;
    case 'l':
      cmd_l(&sys);
      break;
    case 'a':
      cmd_a(&sys, iva_table, iva_count);
      break;
    case 'r':
      cmd_r(&sys, iva_table, iva_count);
      break;
    case 'f':
      cmd_f(&sys, iva_table, iva_count);
      break;
    case 'c':
      cmd_c(&sys);
      break;
    case 'd':
      cmd_d(&sys);
      break;
    }
  }
  clean_all(&sys);
  return 0;
}