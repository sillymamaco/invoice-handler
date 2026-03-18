/**
 * @file main.c
 * @brief Program entry point: IVA table initialisation and command dispatch.
 *
 * @details
 * The IVA table is a fixed array of ::IVA_TABLE_SIZE slots indexed by
 * @c (letter - 'A'). When invoked without arguments the four default rates
 * (A 0 %, B 6 %, C 13 %, D 23 %) are loaded; otherwise the first argument
 * is treated as a path to a text file containing @c "<LETTER> <RATE>" pairs.
 *
 * The command dispatch loop reads one character at a time from @c stdin,
 * skipping whitespace, and calls the matching command handler. The loop
 * terminates on @c 'q' or @c EOF, both of which trigger clean_all() before
 * returning.
 */

#include "commands.h"
#include "common.h"
#include "memory.h"
#include "utils.h"

/**
 * @brief Initialise the system and run the command dispatch loop.
 *
 * @param argc Argument count.
 * @param argv Argument vector; @c argv[1], when present, is the IVA file path.
 * @return @c 0 on normal termination.
 */
int main(int argc, char *argv[]) {
  SystemState sys = {0};
  sys.next_invoice_id = 1;

  /** Zero-initialise all IVA slots (@c present = 0). */
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

  clean_all(&sys);
  return 0;
}