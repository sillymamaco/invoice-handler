/**
 * @file main.c
 * @brief Program entry point: IVA table initialisation and command dispatch.
 * @author IST1117890 (Irina Cojocari)
 */

#include "commands.h"
#include "common.h"
#include "memory.h"
#include "utils.h"

/**
 * @brief Initialise the system and run the command dispatch loop.
 *
 * @param argc Argument count.
 * @param argv Argument vector; argv[1], when present, is the IVA file path.
 * @return 0 on normal termination.
 */
int main(int argc, char *argv[]) {
  SystemState sys = {0};
  sys.next_invoice_id = 1;
  Iva iva_table[IVA_TABLE_SIZE] = {{0, 0}};
  if (argc == 1) {
    /* Default IVA rates when no configuration file is supplied. */
    iva_set(iva_table, 'A', 0);
    iva_set(iva_table, 'B', 6);
    iva_set(iva_table, 'C', 13);
    iva_set(iva_table, 'D', 23);
  } else {
    FILE *f = fopen(argv[1], "r");
    if (f) {
      char letter;
      int value;
      int loaded = 0;
      while (loaded < IVA_TABLE_SIZE &&
             fscanf(f, " %c %d", &letter, &value) == 2) {
        iva_set(iva_table, letter, value);
        loaded++;
      }
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
      cmd_p(&sys, iva_table);
      break;
    case 'l':
      cmd_l(&sys);
      break;
    case 'a':
      cmd_a(&sys, iva_table);
      break;
    case 'r':
      cmd_r(&sys, iva_table);
      break;
    case 'f':
      cmd_f(&sys, iva_table);
      break;
    case 'c':
      cmd_c(&sys);
      break;
    case 'd':
      cmd_d(&sys);
      break;
    default:
      break; /* Unknown commands are silently ignored. */
    }
  }
  /* No memory leaks round here */
  clean_all(&sys);
  return 0;
}
