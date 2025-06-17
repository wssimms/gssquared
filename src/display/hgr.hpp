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

#undef BAZYAR
#ifdef BAZYAR

#pragma once

#include <vector>
#include <cstdint>
#include "gs2.hpp"
#include "cpu.hpp"

#define CHAR_NUM 256
#define CHAR_WIDTH 16
#define CELL_WIDTH 14

typedef enum {
    MODEL_II,
    MODEL_IIJPLUS,
    MODEL_IIE
} Model;

void render_hgrng_scanline(cpu_state *cpu, int y, uint8_t *pixels);
void emitBitSignalHGR(uint8_t *hiresData, uint8_t *bitSignalOut, size_t outputOffset, int y);

#endif