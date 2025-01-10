/*
 *   Copyright (c) 2025 Jawaid Bazyar

 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.

 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.

 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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
