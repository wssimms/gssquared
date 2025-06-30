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

#include <vector>
#include <cstdio>
#include <stdexcept>
#include <cstdint>
#include <algorithm>
#include "display/types.hpp"
#include "display/hgr.hpp"
#include "display/font.hpp"
#include "display/displayng.hpp"
#include "display/ntsc.hpp"
#include "display/display.hpp"
/** 
 * each one of these video mode modules takes Apple II source data, and creates a 560x192 array.
 */

#define DEBUG 0

int hiresMap[24] = {
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

inline int hiresMapIndex(int y) {
    int row1 = y / 8;
    int row2 = y % 8;
    return hiresMap[row1] + row2 * 0x400;
}

/**
 * @brief Processes a single scanline (40 bytes) of Apple II hi-res data
 * 
 * @param hiresData The complete hi-res image data
 * @param startOffset The starting offset in the image data for this scanline
 * @param output Vector to store the processed scanline data
 * @param outputOffset Offset in the output vector to store the data
 * @return The number of bytes written to the output
 */
size_t generateHiresScanline(uint8_t *hiresData, int startOffset, 
                           uint8_t *output, size_t outputOffset) {
    int lastByte = 0x00;
    size_t index = outputOffset;
    
    //printf("hiresData: %p startOffset: %04X\n", hiresData, startOffset);

    // Process 40 bytes (one scanline)
    for (int x = 0; x < 40; x++) {
        uint8_t byte = hiresData[startOffset + x];

        size_t fontIndex = (byte | ((lastByte & 0x40) << 2)) * CHAR_WIDTH; // bit 6 from last byte selects 2nd half of font

        for (int i = 0; i < CELL_WIDTH; i++) {
            output[index++] = hires40Font[fontIndex + i];
            //printf("%02X ", hires40Font[fontIndex + i]);
        }
        
        lastByte = byte;
    }
    
    return index - outputOffset; // Return number of bytes written
}

/**
 * Called with:
 * hiresData: pointer to 8K block in memory of the hi-res image. We will "raw" read this from the memory mapped memory buffer.
 * bitSignalOut: pointer to 560x192 array of uint8_t. This will be filled with the bit signal data.
 * outputOffset: offset in the output array to store the data.
 * y: the y coordinate of the scanline we are processing.
 */
void emitBitSignalHGR(uint8_t *hiresData, uint8_t *bitSignalOut, size_t outputOffset, int y) {
   
    // Get the starting memory offset for this scanline
    int row1 = y / 8;
    int row2 = y % 8;
    int offset = hiresMap[row1] + row2 * 0x400;

    // Process this scanline directly
    //size_t outputOffset = y * pixelsPerScanline;
    size_t bytesWritten = generateHiresScanline(hiresData, offset, bitSignalOut, outputOffset);

}

/**
 * pixels here is the texture update buffer.
 */
void render_hgrng_scanline(cpu_state *cpu, int y, uint8_t *pixels)
{
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    uint8_t *hgrdata = NULL;
    uint8_t *ram = ds->mmu->get_memory_base();

    if (ds->display_page_num == DISPLAY_PAGE_1) {
        //hgrdata = cpu->memory->pages_read[0x20];
        hgrdata = ram + 0x2000; //cpu->mmu->get_page_base_address(0x20);
    } else if (ds->display_page_num == DISPLAY_PAGE_2) {
        //hgrdata = cpu->memory->pages_read[0x40];
        hgrdata = ram + 0x4000; //cpu->mmu->get_page_base_address(0x40);
    } else {
        return;
    }

    for (int yy = y * 8; yy < (y + 1) * 8; yy++) {
        //printf("yy: %d\n", yy);
        emitBitSignalHGR(hgrdata, frameBuffer, yy * 560, yy);
    }
    processAppleIIFrame_LUT(frameBuffer + (y * 8 * 560), (RGBA_t *)pixels, y * 8, (y + 1) * 8);
}
