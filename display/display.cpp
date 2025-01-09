#include <SDL2/SDL.h>

#include "cpu.hpp"
#include "gs2.hpp"
#include "memory.hpp"
#include "debug.hpp"

#include "bus.hpp"
#include "display.hpp"
#include "text_40x24.hpp"
#include "lores_40x48.hpp"
#include "hgr_280x192.hpp"
#include "platforms.hpp"

SDL_Surface* winSurface = NULL;
SDL_Window* window = NULL;

SDL_Renderer* renderer = nullptr;
SDL_Texture* screenTexture = nullptr;

uint32_t dirty_line[24] = {0};

line_mode_t line_mode[24] = {LM_TEXT_MODE}; // 0 = TEXT, 1 = LO RES GRAPHICS, 2 = HI RES GRAPHICS


display_mode_t display_mode = TEXT_MODE;
display_split_mode_t display_split_mode = FULL_SCREEN;
display_graphics_mode_t display_graphics_mode = LORES_MODE;

void set_display_page1() {
    TEXT_PAGE_START = 0x0400;
    TEXT_PAGE_END = 0x07FF;
    TEXT_PAGE_TABLE = TEXT_PAGE_1_TABLE;

    HGR_PAGE_START = 0x2000;
    HGR_PAGE_END = 0x3FFF;
    HGR_PAGE_TABLE = HGR_PAGE_1_TABLE;
}

void set_display_page2() {
    TEXT_PAGE_START = 0x0800;
    TEXT_PAGE_END = 0x0BFF;
    TEXT_PAGE_TABLE = TEXT_PAGE_2_TABLE;

    HGR_PAGE_START = 0x4000;
    HGR_PAGE_END = 0x5FFF;
    HGR_PAGE_TABLE = HGR_PAGE_2_TABLE;

}

uint64_t init_display_sdl(rom_data *rd) {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
        return 1;
    }

    force_display_update();

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

    pre_calculate_font(rd);

    return 0;
}

/**
 * This is effectively a "redraw the entire screen each frame" method now.
 * With an optimization only update dirty lines.
 */
void update_display(cpu_state *cpu) {
    int updated = 0;
    for (int line = 0; line < 24; line++) {
        if (dirty_line[line]) {
            //fprintf(stdout, "Dirty line %d\n", i);
            render_line(cpu, line);
            dirty_line[line] = 0;
            updated = 1;
        }
    }

    if (updated) {
        SDL_RenderCopy(renderer, screenTexture, NULL, NULL);
        SDL_RenderPresent(renderer);
    }
}

void force_display_update() {
    for (int y = 0; y < 24; y++) {
        dirty_line[y] = 1;
    }
}

void free_display() {
    if (screenTexture) SDL_DestroyTexture(screenTexture);
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}


void update_line_mode() {
    line_mode_t top_mode;
    line_mode_t bottom_mode;

    if (display_mode == TEXT_MODE) {
        top_mode = LM_TEXT_MODE;
    } else {
        if (display_graphics_mode == LORES_MODE) {
            top_mode = LM_LORES_MODE;
        } else {
            top_mode = LM_HIRES_MODE;
        }
    }

    if (display_split_mode == SPLIT_SCREEN) {
        bottom_mode = LM_TEXT_MODE;
    } else {
        bottom_mode = top_mode;
    }

    for (int y = 0; y < 20; y++) {
        line_mode[y] = top_mode;
    }
    for (int y = 20; y < 24; y++) {
        line_mode[y] = bottom_mode;
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
        //uint8_t character = raw_memory_read(cpu, TEXT_PAGE_TABLE[y] + x);
        if (line_mode[y] == LM_TEXT_MODE) {
            render_text(cpu, x, y, pixels, pitch);
        } else if (line_mode[y] == LM_LORES_MODE) {
            render_lores(cpu, x, y, pixels, pitch);
        } else if (line_mode[y] == LM_HIRES_MODE) {
            render_hgr(cpu, x, y, pixels, pitch);
        }
    }

    SDL_UnlockTexture(screenTexture);
}


uint8_t txt_bus_read_C050(cpu_state *cpu, uint16_t address) {
    // set graphics mode
    if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Set Graphics Mode\n");
    display_mode = GRAPHICS_MODE;
    update_line_mode();
    force_display_update();
    return 0;
}

void txt_bus_write_C050(cpu_state *cpu, uint16_t address, uint8_t value) {
    txt_bus_read_C050(cpu, address);
}


uint8_t txt_bus_read_C051(cpu_state *cpu, uint16_t address) {
// set text mode
    if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Set Text Mode\n");
    display_mode = TEXT_MODE;
    update_line_mode();
    force_display_update();
    return 0;
}

void txt_bus_write_C051(cpu_state *cpu, uint16_t address, uint8_t value) {
    txt_bus_read_C051(cpu, address);
}


uint8_t txt_bus_read_C052(cpu_state *cpu, uint16_t address) {
    // set full screen
    if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Set Full Screen\n");
    display_split_mode = FULL_SCREEN;
    update_line_mode();
    force_display_update();
    return 0;
}

void txt_bus_write_C052(cpu_state *cpu, uint16_t address, uint8_t value) {
    txt_bus_read_C052(cpu, address);
}


uint8_t txt_bus_read_C053(cpu_state *cpu, uint16_t address) {
    // set split screen
    if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Set Split Screen\n");
    display_split_mode = SPLIT_SCREEN;
    update_line_mode();
    force_display_update();
    return 0;
}
void txt_bus_write_C053(cpu_state *cpu, uint16_t address, uint8_t value) {
    txt_bus_read_C053(cpu, address);
}


uint8_t txt_bus_read_C054(cpu_state *cpu, uint16_t address) {
    // switch to screen 1
    if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Switching to screen 1\n");
    set_display_page1();
    force_display_update();
    return 0;
}
void txt_bus_write_C054(cpu_state *cpu, uint16_t address, uint8_t value) {
    txt_bus_read_C054(cpu, address);
}


uint8_t txt_bus_read_C055(cpu_state *cpu, uint16_t address) {
    // switch to screen 2
    if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Switching to screen 2\n");
    set_display_page2();
    force_display_update();
    return 0;
}

void txt_bus_write_C055(cpu_state *cpu, uint16_t address, uint8_t value) {
    txt_bus_read_C055(cpu, address);
}


uint8_t txt_bus_read_C056(cpu_state *cpu, uint16_t address) {
    // set lo-res (graphics) mode
    if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Set Lo-Res Mode\n");
    display_graphics_mode = LORES_MODE;
    update_line_mode();
    force_display_update();
    return 0;
}

void txt_bus_write_C056(cpu_state *cpu, uint16_t address, uint8_t value) {
    txt_bus_read_C056(cpu, address);
}


uint8_t txt_bus_read_C057(cpu_state *cpu, uint16_t address) {
    // set hi-res (graphics) mode
    if (DEBUG(DEBUG_DISPLAY)) fprintf(stdout, "Set Hi-Res Mode\n");
    display_graphics_mode = HIRES_MODE;
    update_line_mode();
    force_display_update();
    return 0;
}

void txt_bus_write_C057(cpu_state *cpu, uint16_t address, uint8_t value) {
    txt_bus_read_C057(cpu, address);
}

void init_device_display() {
    register_C0xx_memory_read_handler(0xC050, txt_bus_read_C050);
    register_C0xx_memory_read_handler(0xC051, txt_bus_read_C051);
    register_C0xx_memory_read_handler(0xC052, txt_bus_read_C052);
    register_C0xx_memory_read_handler(0xC053, txt_bus_read_C053);
    register_C0xx_memory_read_handler(0xC054, txt_bus_read_C054);
    register_C0xx_memory_read_handler(0xC055, txt_bus_read_C055);
    register_C0xx_memory_read_handler(0xC056, txt_bus_read_C056);
    register_C0xx_memory_read_handler(0xC057, txt_bus_read_C057);

    register_C0xx_memory_write_handler(0xC050, txt_bus_write_C050);
    register_C0xx_memory_write_handler(0xC051, txt_bus_write_C051);
    register_C0xx_memory_write_handler(0xC052, txt_bus_write_C052);
    register_C0xx_memory_write_handler(0xC053, txt_bus_write_C053);
    register_C0xx_memory_write_handler(0xC054, txt_bus_write_C054);
    register_C0xx_memory_write_handler(0xC055, txt_bus_write_C055);
    register_C0xx_memory_write_handler(0xC056, txt_bus_write_C056);
    register_C0xx_memory_write_handler(0xC057, txt_bus_write_C057);
}
