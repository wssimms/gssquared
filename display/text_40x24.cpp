#include <cstdio>
#include <unistd.h>
#include <cstdlib>

#include "../gs2.hpp"
#include "../devices/keyboard.hpp"
#include "../memory.hpp"
#include "../debug.hpp"
#include "../bus.hpp"

// Apple II+ Character Set (7x8 pixels)
// Characters 0x20 through 0x7F
#define CHAR_GLYPHS_COUNT 256
#define CHAR_GLYPHS_SIZE 8

// The Character ROM contains 256 entries (total of 2048 bytes).
// Each character is 8 bytes, each byte is 1 row of pixels.
uint8_t APPLE2_FONT[CHAR_GLYPHS_COUNT * CHAR_GLYPHS_SIZE];

uint32_t APPLE2_FONT_32[CHAR_GLYPHS_COUNT * CHAR_GLYPHS_SIZE * 8 ]; // 8 pixels per row.

/** pre-render the text mode font into a 32-bit bitmap - directly ready to shove into texture*/
/**
 * input: APPLE2_FONT 
 * output: APPLE2_FONT_32
*/

void pre_calculate_font () {
    // Draw the character bitmap into the texture
    // pitch is the number of pixels per row of the texture. May depend on device.
    // we don't use pitch here.
    // each char will be 56 bytes.

    uint32_t fg_color = 0xFFFFFFFF;
    uint32_t bg_color = 0x00000000;

    uint32_t *position = APPLE2_FONT_32;

    for (int row = 0; row < 256 * 8; row++) {
        uint8_t rowBits = APPLE2_FONT[row];
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

#include <SDL2/SDL.h>

SDL_Surface* winSurface = NULL;
SDL_Window* window = NULL;

SDL_Renderer* renderer = nullptr;
SDL_Texture* screenTexture = nullptr;

uint32_t dirty_line[24];

void force_text_display_update() {
    for (int y = 0; y < 24; y++) {
        dirty_line[y] = 1;
    }
}

uint64_t init_display() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
        return 1;
    }

    force_text_display_update();

    const int SCALE_X = 4;
    const int SCALE_Y = 4;
    const int BASE_WIDTH = 280;
    const int BASE_HEIGHT = 192;
    
    window = SDL_CreateWindow(
        "GSSquared - Apple ][ Emulator", 
        SDL_WINDOWPOS_UNDEFINED, 
        SDL_WINDOWPOS_UNDEFINED, 
        BASE_WIDTH * SCALE_X, 
        BASE_HEIGHT * SCALE_Y, 
        SDL_WINDOW_SHOWN
    );

    if (!window) {
        fprintf(stderr, "Error creating window: %s\n", SDL_GetError());
        return 1;
    }

    // Create renderer with nearest-neighbor scaling (sharp pixels)
    renderer = SDL_CreateRenderer(window, -1, 
        SDL_RENDERER_ACCELERATED /* | SDL_RENDERER_PRESENTVSYNC */);
    
    if (!renderer) {
        fprintf(stderr, "Error creating renderer: %s\n", SDL_GetError());
        return 1;
    }

    // Set scaling quality to nearest neighbor for sharp pixels
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");
    
    // Create the screen texture
    screenTexture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        280, 192);

    if (!screenTexture) {
        fprintf(stderr, "Error creating screen texture: %s\n", SDL_GetError());
        return 1;
    }

    // Clear the texture to black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    SDL_RaiseWindow(window);

    load_character_rom();

    return 0;
}

bool flash_state = false;

/**
 * render single character - in context of a locked texture of a whole display line.
 */
void render_character(int x, int y, void *pixels, int pitch, uint8_t character) {
    // Bounds checking
    if (x < 0 || x >= 40 || y < 0 || y >= 24) {
        return;
    }

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

void render_line(cpu_state *cpu, int y) {

    SDL_Rect updateRect = {
        0,    // X position (left of window))
        y * 8,    // Y position (8 pixels per character)
        280,        // Width of line
        8         // Height of line
    };

    // the texture is our canvas. When we 'lock' it, we get a pointer to the pixels, and the pitch which is pixels per row
    // of the area. Since all our chars are the same we can just use the same pitch for all our chars.

    void* pixels;
    int pitch;
    if (SDL_LockTexture(screenTexture, &updateRect, &pixels, &pitch) < 0) {
        fprintf(stderr, "Failed to lock texture: %s\n", SDL_GetError());
        return;
    }

    for (int x = 0; x < 40; x++) {
        uint8_t character = raw_memory_read(cpu, TEXT_PAGE_TABLE[y] + x);
        render_character(x, y, pixels, pitch, character);
    }

    SDL_UnlockTexture(screenTexture);
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
                //render_character(x, y, character, flash_state);
                //flashcount++;
            }
        }
    }
    //if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Flash chars updated: %d\n", flashcount);
}

/**
 * This is effectively a "redraw the entire screen each frame" method now.
 * With an optimization only update dirty lines.
 */
void update_display(cpu_state *cpu) {
    int updated = 0;
    for (int i = 0; i < 24; i++) {
        if (dirty_line[i]) {
            //fprintf(stdout, "Dirty line %d\n", i);
            render_line(cpu, i);
            dirty_line[i] = 0;
            updated = 1;
        }
    }

    if (updated) {
        SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
}

void free_display() {
    if (screenTexture) SDL_DestroyTexture(screenTexture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

// write a character to the display memory based on the text page memory address.
// converts address to an X,Y coordinate then calls render_character to do it.
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

    dirty_line[y_loc] = 1;
}

void set_text_page1() {
    TEXT_PAGE_START = 0x0400;
    TEXT_PAGE_END = 0x07FF;
    TEXT_PAGE_TABLE = TEXT_PAGE_1_TABLE;
}

void set_text_page2() {
    TEXT_PAGE_START = 0x0800;
    TEXT_PAGE_END = 0x0BFF;
    TEXT_PAGE_TABLE = TEXT_PAGE_2_TABLE;
}

uint8_t txt_bus_read(cpu_state *cpu, uint16_t address) {
    return 0;
}

void txt_bus_write(cpu_state *cpu, uint16_t address, uint8_t value) {
    if (address == 0xC054) {
        // switch to screen 1
        if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Switching to screen 1\n");
        set_text_page1();
        force_text_display_update();
    }
    if (address == 0xC055) {
        // switch to screen 2
        if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Switching to screen 2\n");
        set_text_page2();
        force_text_display_update();
    }
}

void init_text_40x24_display() {
    register_C0xx_memory_write_handler(0xC054, txt_bus_write);
    register_C0xx_memory_write_handler(0xC055, txt_bus_write);
}
