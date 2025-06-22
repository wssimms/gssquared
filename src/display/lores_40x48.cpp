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


#include "gs2.hpp"
#include "cpu.hpp"
#include "display.hpp"
#include "text_40x24.hpp"
#include "lores_40x48.hpp"
#include "devices/displaypp/RGBA.hpp"

RGBA_t lores_color_table[16] = {
    {0x00, 0x00, 0x00, 0xFF},
    {0x8A, 0x21, 0x40, 0xFF},
    {0x3C, 0x22, 0xA5, 0xFF},
    {0xC8, 0x47, 0xE4, 0xFF},
    {0x07, 0x65, 0x3E, 0xFF},
    {0x7B, 0x7E, 0x80, 0xFF},
    {0x30, 0x8E, 0xF3, 0xFF},
    {0xB9, 0xA9, 0xFD, 0xFF},
    {0x3B, 0x51, 0x07, 0xFF},
    {0xC7, 0x70, 0x28, 0xFF},
    {0x7B, 0x7E, 0x80, 0xFF},
    {0xF3, 0x9A, 0xC2, 0xFF},
    {0x2F, 0xB8, 0x1F, 0xFF},
    {0xB9, 0xD0, 0x60, 0xFF},
    {0x6E, 0xE1, 0xC0, 0xFF},
    {0xFF, 0xFF, 0xFF, 0xFF}
    };

void render_lores_scanline(cpu_state *cpu, int y, void *pixels, int pitch) {
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    display_page_t *display_page = ds->display_page_table;
    uint16_t *TEXT_PAGE_TABLE = display_page->text_page_table;

    for (int x = 0; x < 40; x++) {
        //uint8_t character = raw_memory_read(cpu, TEXT_PAGE_TABLE[y] + x);
        uint8_t character = cpu->mmu->read_raw(TEXT_PAGE_TABLE[y] + x);

        // look up color key for top and bottom block
        RGBA_t color_top = lores_color_table[character & 0x0F];
        RGBA_t color_bottom = lores_color_table[(character & 0xF0) >> 4];

        int pitchoff = pitch / 4;
        int charoff = x * 14;
        // Draw the character bitmap into the texture
        RGBA_t * texturePixels = (RGBA_t *)pixels;
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