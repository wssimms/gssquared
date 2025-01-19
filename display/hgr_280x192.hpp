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

extern uint16_t HGR_PAGE_START;
extern uint16_t HGR_PAGE_END;
extern uint16_t *HGR_PAGE_TABLE;

extern uint16_t HGR_PAGE_1_TABLE[24];
extern uint16_t HGR_PAGE_2_TABLE[24];

void render_hgr(cpu_state *cpu, int x, int y, void *pixels, int pitch);
void hgr_memory_write(cpu_state *cpu, uint16_t address, uint8_t value);
