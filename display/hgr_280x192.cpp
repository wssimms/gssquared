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
#include "hgr_280x192.hpp"
#include "memory.hpp"
#include "debug.hpp"
#include "display.hpp"

uint16_t HGR_PAGE_1_TABLE[24] = {
    0x2000,
    0x2080,
    0x2100,
    0x2180,
    0x2200,
    0x2280,
    0x2300,
    0x2380,

    0x2028,
    0x20A8,
    0x2128,
    0x21A8,
    0x2228,
    0x22A8,
    0x2328,
    0x23A8,

    0x2050,
    0x20D0,
    0x2150,
    0x21D0,
    0x2250,
    0x22D0,
    0x2350,
    0x23D0,
};

uint16_t HGR_PAGE_2_TABLE[24] = {
    0x4000,
    0x4080,
    0x4100,
    0x4180,
    0x4200,
    0x4280,
    0x4300,
    0x4380,

    0x4028,
    0x40A8,
    0x4128,
    0x41A8,
    0x4228,
    0x42A8,
    0x4328,
    0x43A8,

    0x4050,
    0x40D0,
    0x4150,
    0x41D0,
    0x4250,
    0x42D0,
    0x4350,
    0x43D0,
};

uint32_t hgr_mono_color_table[2] = {
    0x00000000,
    0x009933FF,
};

uint16_t HGR_PAGE_START = 0x2000;
uint16_t HGR_PAGE_END = 0x3FFF;
uint16_t *HGR_PAGE_TABLE = HGR_PAGE_1_TABLE;

void render_hgr(cpu_state *cpu, int x, int y, void *pixels, int pitch) {
    //if (DEBUG(DEBUG_HGR)) fprintf(stdout, "render_hgr x: %d y: %d\n", x, y);

    // Bounds checking
    if (x < 0 || x >= 40 || y < 0 || y >= 24) {
        return;
    }

    // look up color key for top and bottom block
    
    int pitchoff = pitch / 4;
    int charoff = x * 7;
    // Draw the character bitmap into the texture
    uint32_t* texturePixels = (uint32_t*)pixels;
    for (int row = 0; row < 8; row++) {
        uint16_t address = HGR_PAGE_TABLE[y] + (row * 0x0400) + x;
        uint8_t character = raw_memory_read(cpu, address);

        uint32_t base = row * pitchoff;
        //if (DEBUG(DEBUG_HGR)) fprintf(stdout, "row: %d base: %d character: %02X\n", row, base, character);
        for (int bit = 0; bit < 7; bit++) {
            uint32_t pixel = hgr_mono_color_table[character & 0x01];
            texturePixels[base + charoff + bit] = pixel;
            character >>= 1;
        }
    }
}

void hgr_memory_write(uint16_t address, uint8_t value) {
    // Skip unless we're in graphics mode.
    if (! (display_mode == GRAPHICS_MODE && display_graphics_mode == HIRES_MODE)) {
        return;
    }
    if (DEBUG(DEBUG_HGR)) fprintf(stdout, "hgr_memory_write address: %04X value: %02X\n", address, value);
    // Strict bounds checking for text page 1
    if (address < HGR_PAGE_START || address > HGR_PAGE_END) {
        return;
    }

    // Convert text memory address to screen coordinates
    uint16_t addr_rel = (address - HGR_PAGE_START) & 0x03FF;
    
    // Each superrow is 128 bytes apart
    uint8_t superrow = addr_rel >> 7;      // Divide by 128 to get 0 or 1
    uint8_t superoffset = addr_rel & 0x7F; // Get offset within the 128-byte block
    
    uint8_t subrow = superoffset / 40;     // Each row is 40 characters
    uint8_t charoffset = superoffset % 40;
    
    // Calculate final screen position
    uint8_t y_loc = (subrow * 8) + superrow; // Each superrow contains 8 rows
    uint8_t x_loc = charoffset;

    if (DEBUG(DEBUG_HGR)) fprintf(stdout, "Address: $%04X -> dirty line y:%d (value: $%02X)\n", 
           address, y_loc, value); // Debug output

    // Extra bounds verification
    if (x_loc >= 40 || y_loc >= 24) {
        if (DEBUG(DEBUG_HGR)) fprintf(stdout, "Invalid coordinates calculated: x=%d, y=%d from addr=$%04X\n", 
               x_loc, y_loc, address);
        return;
    }

    dirty_line[y_loc] = 1;
}