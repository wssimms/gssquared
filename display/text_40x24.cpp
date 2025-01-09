#include <cstdio>
#include <unistd.h>
#include <cstdlib>

#include "gs2.hpp"
#include "memory.hpp"
#include "debug.hpp"
#include "bus.hpp"
#include "display.hpp"
#include "platforms.hpp"


// Apple II+ Character Set (7x8 pixels)
// Characters 0x20 through 0x7F
#define CHAR_GLYPHS_COUNT 256
#define CHAR_GLYPHS_SIZE 8

// The Character ROM contains 256 entries (total of 2048 bytes).
// Each character is 8 bytes, each byte is 1 row of pixels.
//uint8_t APPLE2_FONT[CHAR_GLYPHS_COUNT * CHAR_GLYPHS_SIZE];

uint32_t APPLE2_FONT_32[CHAR_GLYPHS_COUNT * CHAR_GLYPHS_SIZE * 8 ]; // 8 pixels per row.

/** pre-render the text mode font into a 32-bit bitmap - directly ready to shove into texture*/
/**
 * input: APPLE2_FONT 
 * output: APPLE2_FONT_32
*/

void pre_calculate_font (rom_data *rd) {
    // Draw the character bitmap into the texture
    // pitch is the number of pixels per row of the texture. May depend on device.
    // we don't use pitch here.
    // each char will be 56 bytes.

    uint32_t fg_color = 0xFFFFFFFF;
    uint32_t bg_color = 0x00000000;

    uint32_t *position = APPLE2_FONT_32;

    for (int row = 0; row < 256 * 8; row++) {
        uint8_t rowBits = rd->char_rom[row];
        //fprintf(stdout, "position: %8p, row: %04X, rowBits: %02X   ", position, row, rowBits);
        for (int col = 0; col < 7; col++) {
            bool pixel = rowBits & (1 << (6 - col));
            uint32_t color = pixel ? fg_color : bg_color;
            //fprintf(stdout, "%08X", color);
            *position = color;
            position++;
        }
        //fprintf(stdout, "\n");
    }
}

#if 0
void load_character_rom() {

    FILE* rom = fopen("roms/apple2_plus/Apple II Character ROM - 341-0036.bin", "rb");
    if (!rom) {
        fprintf(stderr, "Error: Could not open character ROM file\n");
        exit(1);
    }

    // Read the entire 2KB ROM file
    if (fread((void*)APPLE2_FONT, 1, CHAR_GLYPHS_COUNT * CHAR_GLYPHS_SIZE, rom) != CHAR_GLYPHS_COUNT * CHAR_GLYPHS_SIZE) {
        fprintf(stderr, "Error: Could not read character ROM file\n");
        fclose(rom);
        exit(1);
    }

    fclose(rom);

    pre_calculate_font();
}
#endif

uint16_t TEXT_PAGE_1_TABLE[24] = {
    0x0400,
    0x0480,
    0x0500,
    0x0580,
    0x0600,
    0x0680,
    0x0700,
    0x0780,

    0x0428,
    0x04A8,
    0x0528,
    0x05A8,
    0x0628,
    0x06A8,
    0x0728,
    0x07A8,

    0x0450,
    0x04D0,
    0x0550,
    0x05D0,
    0x0650,
    0x06D0,
    0x0750,
    0x07D0,
};

uint16_t TEXT_PAGE_2_TABLE[24] = {
    0x0800,
    0x0880,
    0x0900,
    0x0980,
    0x0A00,
    0x0A80,
    0x0B00,
    0x0B80,

    0x0828,
    0x08A8,
    0x0928,
    0x09A8,
    0x0A28,
    0x0AA8,
    0x0B28,
    0x0BA8,

    0x0850,
    0x08D0,
    0x0950,
    0x09D0,
    0x0A50,
    0x0AD0,
    0x0B50,
    0x0BD0,
};

// these three variables define the current text page
// when we switch to screen 2, we change TEXT_PAGE_START, TEXT_PAGE_END, and TEXT_PAGE_TABLE
// default to screen 1
uint16_t TEXT_PAGE_START = 0x0400;
uint16_t TEXT_PAGE_END = 0x07FF;
uint16_t *TEXT_PAGE_TABLE = TEXT_PAGE_1_TABLE;


bool flash_state = false;

/**
 * render single character - in context of a locked texture of a whole display line.
 */
void render_text(cpu_state *cpu, int x, int y, void *pixels, int pitch) {
    // Bounds checking
    if (x < 0 || x >= 40 || y < 0 || y >= 24) {
        return;
    }

    uint8_t character = raw_memory_read(cpu, TEXT_PAGE_TABLE[y] + x);

    // Calculate font offset (8 bytes per character, starting at 0x20)
    const uint32_t* charPixels = &APPLE2_FONT_32[character * 56];

    bool inverse = false;
    // Check if top two bits are 0 (0x00-0x3F range)
    if ((character & 0xC0) == 0) {
        inverse = true;
    } else if (((character & 0xC0) == 0x40)) {
        inverse = flash_state;
    }
    
    // for inverse, xor the pixels with 0xFFFFFFFF to invert them.
    uint32_t xor_mask = 0x00000000;
    if (inverse) {
        xor_mask = 0xFFFFFFFF;
    }

    int pitchoff = pitch / 4;
    int charoff = x * 7;
    // Draw the character bitmap into the texture
    uint32_t* texturePixels = (uint32_t*)pixels;
    for (int row = 0; row < 8; row++) {
        uint32_t base = row * pitchoff;
        texturePixels[base + charoff ] = charPixels[0] ^ xor_mask;
        texturePixels[base + charoff + 1] = charPixels[1] ^ xor_mask;
        texturePixels[base + charoff + 2] = charPixels[2] ^ xor_mask;
        texturePixels[base + charoff + 3] = charPixels[3] ^ xor_mask;
        texturePixels[base + charoff + 4] = charPixels[4] ^ xor_mask;
        texturePixels[base + charoff + 5] = charPixels[5] ^ xor_mask;
        texturePixels[base + charoff + 6] = charPixels[6] ^ xor_mask;
        charPixels += 7;
    }
}


void update_flash_state(cpu_state *cpu) {
    // 2 times per second (every 30 frames), the state of flashing characters (those matching 0b01xxxxxx) must be reversed.
    static int flash_counter = 0;
    flash_counter++;
    if (flash_counter > 30) {
        flash_counter = 0;
        flash_state = !flash_state;
    }
    //int flashcount = 0;
    for (int x = 0; x < 40; x++) {
        for (int y = 0; y < 24; y++) {
            uint16_t addr = TEXT_PAGE_TABLE[y] + x;
            uint8_t character = raw_memory_read(cpu, addr);
            if ((character & 0b11000000) == 0x40) {
                // mark line as dirty
                dirty_line[y] = 1;
                //render_text(x, y, character, flash_state);
                //flashcount++;
            }
        }
    }
    //if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Flash chars updated: %d\n", flashcount);
}


// write a character to the display memory based on the text page memory address.
// converts address to an X,Y coordinate then calls render_text to do it.
void txt_memory_write(uint16_t address, uint8_t value) {

    // Strict bounds checking for text page 1
    if (address < TEXT_PAGE_START || address > TEXT_PAGE_END) {
        return;
    }

    // Convert text memory address to screen coordinates
    uint16_t addr_rel = address - TEXT_PAGE_START;
    
    // Each superrow is 128 bytes apart
    uint8_t superrow = addr_rel >> 7;      // Divide by 128 to get 0 or 1
    uint8_t superoffset = addr_rel & 0x7F; // Get offset within the 128-byte block
    
    uint8_t subrow = superoffset / 40;     // Each row is 40 characters
    uint8_t charoffset = superoffset % 40;
    
    // Calculate final screen position
    uint8_t y_loc = (subrow * 8) + superrow; // Each superrow contains 8 rows
    uint8_t x_loc = charoffset;

    if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Address: $%04X -> dirty line y:%d (value: $%02X)\n", 
           address, y_loc, value); // Debug output

    // Extra bounds verification
    if (x_loc >= 40 || y_loc >= 24) {
        if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Invalid coordinates calculated: x=%d, y=%d from addr=$%04X\n", 
               x_loc, y_loc, address);
        return;
    }

    if (display_mode == GRAPHICS_MODE && display_split_mode == SPLIT_SCREEN) {
        // update lines 21 - 24
        if (y_loc >= 20 && y_loc < 24) {
            dirty_line[y_loc] = 1;
        }
    }

    // update any line.
    dirty_line[y_loc] = 1;
}



uint8_t txt_bus_read(cpu_state *cpu, uint16_t address) {
    return 0;
}


