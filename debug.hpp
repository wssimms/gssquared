#pragma once

// Values can be 0 - no debug, 1 - instruction decode, 2- detailed decode.
extern uint64_t debug_level;

#define DEBUG_OPCODE 0x0001
#define DEBUG_OPERAND 0x0002
#define DEBUG_REGISTERS 0x0004
#define DEBUG_KEYBOARD 0x0010
#define DEBUG_DISPLAY 0x0020
#define DEBUG_CLOCK 0x0040
#define DEBUG_TIMEDRUN 0x0080
#define DEBUG_GUI 0x0100

#define DEBUG_ANY 0xFFFF
#define DEBUG_BOOT_FLAG DEBUG_KEYBOARD 

#define DEBUG(flag) (debug_level & flag)

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

void debug_set_level(uint64_t level);

std::string int_to_hex(uint16_t value) ;
std::string int_to_hex(uint8_t value) ;
inline void print_hex(uint8_t value) ;
