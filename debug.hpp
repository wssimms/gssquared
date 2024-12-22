#pragma once

#define DEBUG 1

#include <cstdint>
#include <iostream>
#include <sstream>
#include <iomanip>
#include "cpu.hpp"

/* void debug_print_cpu(cpu_t *cpu);
 */
void debug_dump_memory(cpu_state *cpu, uint32_t start, uint32_t end);
/* void debug_print_registers(cpu_t *cpu);
void debug_print_flags(cpu_t *cpu);
 */

std::string int_to_hex(uint16_t value) ;
std::string int_to_hex(uint8_t value) ;
inline void print_hex(uint8_t value) ;
