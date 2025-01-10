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


 uint8_t raw_memory_read(cpu_state *cpu, uint16_t address);
 void raw_memory_write(cpu_state *cpu, uint16_t address, uint8_t value);
 void raw_memory_write_word(cpu_state *cpu, uint16_t address, uint16_t value);

uint8_t read_memory(cpu_state *cpu, uint16_t address);
void write_memory(cpu_state *cpu, uint16_t address, uint8_t value);
void memory_bus_read(uint16_t address, uint8_t value);
void memory_bus_write(uint16_t address, uint8_t value);
uint8_t read_byte(cpu_state *cpu, uint16_t address);
uint16_t read_word(cpu_state *cpu, uint16_t address);
uint16_t read_word_from_pc(cpu_state *cpu);
void write_byte(cpu_state *cpu, uint16_t address, uint8_t value);
void write_word(cpu_state *cpu, uint16_t address, uint16_t value);
void store_byte(cpu_state *cpu, uint16_t address, uint8_t value);
void store_word(cpu_state *cpu, uint16_t address, uint16_t value);
 uint8_t read_byte_from_pc(cpu_state *cpu);
