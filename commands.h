/**
 * @file commands.h
 * @brief Prototypes for system commands.
 */

#ifndef COMMANDS_H
#define COMMANDS_H

#include "common.h"

void cmd_p(SystemState *sys, Iva table[], int iva_count);
void cmd_l(SystemState *sys);
void cmd_a(SystemState *sys, Iva table[], int iva_count);
void cmd_f(SystemState *sys, Iva table[], int iva_count);
void cmd_r(SystemState *sys, Iva table[], int iva_count);
void cmd_c(SystemState *sys);
void cmd_d(SystemState *sys);

#endif