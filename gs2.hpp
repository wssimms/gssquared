#pragma once

#include "cpu.hpp"

#define USE_SDL2 1

#define BRK_VECTOR 0xFFFE
#define IRQ_VECTOR 0xFFFE
#define NMI_VECTOR 0xFFFA
#define RESET_VECTOR 0xFFFC

void run_cpus(void) ;
void cpu_reset(cpu_state *cpu) ;
