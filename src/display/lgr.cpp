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

#include <vector>
#include <cstdio>
#include <stdexcept>
#include <cstdint>
#include <algorithm>
#include "cpu.hpp"
#include "types.hpp"
#include "lgr.hpp"
#include "display.hpp"
#include "display/displayng.hpp"
#include "display/ntsc.hpp"
#include "display/display.hpp"
#include "display/types.hpp"

int loresMapIndex[24] =
  {   // text page 1 line addresses
            0x0000,
            0x0080,
            0x0100,
            0x0180,
            0x0200,
            0x0280,
            0x0300,
            0x0380,

            0x0028,
            0x00A8,
            0x0128,
            0x01A8,
            0x0228,
            0x02A8,
            0x0328,
            0x03A8,

            0x0050,
            0x00D0,
            0x0150,
            0x01D0,
            0x0250,
            0x02D0,
            0x0350,
            0x03D0,
        };

#define DEBUG 1

size_t generateLoresScanline(uint8_t *loresData, int startOffset, 
                           uint8_t *output, size_t outputOffset) {
    int lastByte = 0x00;
    size_t index = outputOffset;
    
    // Process 40 bytes (one scanline)
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 40; x++) {
            uint8_t byte = loresData[startOffset + x];

            if (y & 4) { // if we're in the second half of the scanline, shift the byte right 4 bits to get the other nibble
                byte = byte >> 4;
            }
            int pixeloff = (x * 14) % 4;

            for (int bits = 0; bits < 14; bits++) {
                uint8_t bit = ((byte >> pixeloff) & 0x01);
                uint8_t gray = (bit ? 0xFF : 0x00);
                output[index++] = gray;
                pixeloff = (pixeloff + 1) % 4;
            }
        }
    }
    
    return index - outputOffset; // Return number of bytes written
}

void render_lgrng_scanline(cpu_state *cpu, int y)
{
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    const int pixelsPerScanline = 40 * CELL_WIDTH;
    uint8_t *lgrdata = NULL;

    if (ds->display_page_num == DISPLAY_PAGE_1) {
        //lgrdata = cpu->memory->pages_read[0x04];
        lgrdata = cpu->mmu->get_page_base_address(0x04);
    } else if (ds->display_page_num == DISPLAY_PAGE_2) {
        //lgrdata = cpu->memory->pages_read[0x08];
        lgrdata = cpu->mmu->get_page_base_address(0x08);
    } else {
        return;
    }
    int offset = loresMapIndex[y];
    size_t outputOffset = y * pixelsPerScanline * 8;

    // Generate the bitstream for the lores line
    generateLoresScanline(lgrdata, offset, frameBuffer, outputOffset);

    // this processes scanlines in a range of y_start to y_end
    //processAppleIIFrame_LUT(frameBuffer + (y * 8 * 560), (RGBA *)pixels, y * 8, (y + 1) * 8); // convert to color
}

/* uint8_t *loresToGray(uint8_t *loresData) {
    // Calculate output size and pre-allocate
    const int pixelsPerScanline = 40 * CELL_WIDTH;
    const size_t totalPixels = 192 * pixelsPerScanline;
    uint8_t *bitSignalOut = new uint8_t[totalPixels];
    
    // Process each scanline (0-191)
    for (int y = 0; y < 24; y++) {
        // Get the starting memory offset for this scanline
        int offset = loresMapIndex[y];
        
        // Process this scanline directly
        size_t outputOffset = y * pixelsPerScanline * 8;
        size_t bytesWritten = generateLoresScanline(loresData, offset, bitSignalOut, outputOffset);
        
        // Print progress every 20 scanlines
        if (DEBUG ) {
            printf("Processed scanline %d at offset %04X\n", y, offset);
        }
    }
    
    printf("Processed entire image: %zu pixels\n", totalPixels);
    return bitSignalOut;
} */

#endif