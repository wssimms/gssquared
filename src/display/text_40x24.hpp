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

#include "cpu.hpp"

/* extern uint16_t TEXT_PAGE_START;
extern uint16_t TEXT_PAGE_END;
extern uint16_t *TEXT_PAGE_TABLE;
extern uint16_t TEXT_PAGE_1_TABLE[24];
extern uint16_t TEXT_PAGE_2_TABLE[24]; */

void txt_memory_write(cpu_state *, uint16_t , uint8_t );
void update_flash_state(cpu_state *cpu);
void render_text_scanline(cpu_state *cpu, int y, void *pixels, int pitch);
