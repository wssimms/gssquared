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

#include "gs2.hpp"
#include "cpu.hpp"
#include "display.hpp"
#include "text_40x24.hpp"
#include "lores_40x48.hpp"

uint32_t lores_color_table[16] = {
    0x00000000,
    0x8A2140FF,
    0x3C22A5FF,
    0xC847E4FF,
    0x07653EFF,
    0x7B7E80FF,
    0x308EF3FF,
    0xB9A9FDFF,
    0x3B5107FF,
    0xC77028FF,
    0x7B7E80FF,
    0xF39AC2FF,
    0x2FB81FFF,
    0xB9D060FF,
    0x6EE1C0FF,
    0xFFFFFFFF
    };

void render_lores_scanline(cpu_state *cpu, int y, void *pixels, int pitch) {
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    display_page_t *display_page = ds->display_page_table;
    uint16_t *TEXT_PAGE_TABLE = display_page->text_page_table;

    for (int x = 0; x < 40; x++) {
        //uint8_t character = raw_memory_read(cpu, TEXT_PAGE_TABLE[y] + x);
        uint8_t character = cpu->mmu->read_raw(TEXT_PAGE_TABLE[y] + x);

        // look up color key for top and bottom block
        uint32_t color_top = lores_color_table[character & 0x0F];
        uint32_t color_bottom = lores_color_table[(character & 0xF0) >> 4];

        int pitchoff = pitch / 4;
        int charoff = x * 14;
        // Draw the character bitmap into the texture
        uint32_t* texturePixels = (uint32_t*)pixels;
        for (int row = 0; row < 4; row++) {
            uint32_t base = row * pitchoff;
            texturePixels[base + charoff ] = color_top;
            texturePixels[base + charoff + 1] = color_top;
            texturePixels[base + charoff + 2] = color_top;
            texturePixels[base + charoff + 3] = color_top;
            texturePixels[base + charoff + 4] = color_top;
            texturePixels[base + charoff + 5] = color_top;
            texturePixels[base + charoff + 6] = color_top;
            texturePixels[base + charoff + 7] = color_top;
            texturePixels[base + charoff + 8] = color_top;
            texturePixels[base + charoff + 9] = color_top;
            texturePixels[base + charoff + 10] = color_top;
            texturePixels[base + charoff + 11] = color_top;
            texturePixels[base + charoff + 12] = color_top;
            texturePixels[base + charoff + 13] = color_top;
        }

        for (int row = 4; row < 8; row++) {
            uint32_t base = row * pitchoff;
            texturePixels[base + charoff ] = color_bottom;
            texturePixels[base + charoff + 1] = color_bottom;
            texturePixels[base + charoff + 2] = color_bottom;
            texturePixels[base + charoff + 3] = color_bottom;
            texturePixels[base + charoff + 4] = color_bottom;
            texturePixels[base + charoff + 5] = color_bottom;
            texturePixels[base + charoff + 6] = color_bottom;
            texturePixels[base + charoff + 7] = color_bottom;
            texturePixels[base + charoff + 8] = color_bottom;
            texturePixels[base + charoff + 9] = color_bottom;
            texturePixels[base + charoff + 10] = color_bottom;
            texturePixels[base + charoff + 11] = color_bottom;
            texturePixels[base + charoff + 12] = color_bottom;
            texturePixels[base + charoff + 13] = color_bottom;
        }    
    }
}

#endif