#pragma once

#include <stdint.h>
#include "cpu.hpp"

// Function pointer type for memory bus handlers
typedef uint8_t (*memory_read_handler)(cpu_state *, uint16_t);
typedef void (*memory_write_handler)(cpu_state *, uint16_t, uint8_t);

uint8_t memory_bus_read(cpu_state *cpu, uint16_t address);
void memory_bus_write(cpu_state *cpu, uint16_t address, uint8_t value);

void register_C0xx_memory_read_handler(uint16_t address, memory_read_handler handler);
void register_C0xx_memory_write_handler(uint16_t address, memory_write_handler handler);
