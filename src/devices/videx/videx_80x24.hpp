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

#include "videx.hpp"

void videx_memory_write(cpu_state *cpu, SlotType_t slot, uint16_t address, uint8_t value);
uint8_t videx_memory_read(cpu_state *cpu, SlotType_t slot, uint16_t address);
void videx_memory_write2(void *context, uint16_t address, uint8_t value);
uint8_t videx_memory_read2(void *context, uint16_t address);
