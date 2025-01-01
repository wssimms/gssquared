#pragma once

#include "../gs2.hpp"
#include "../cpu.hpp"

// Graphics vs Text, C050 / C051
typedef enum {
    TEXT_MODE = 0,
    GRAPHICS_MODE = 1,
} display_mode_t;

// Full screen vs split screen, C052 / C053
typedef enum {
    FULL_SCREEN = 0,
    SPLIT_SCREEN = 1,
} display_split_mode_t;

// Lo-res vs Hi-res, C056 / C057
typedef enum {
    LORES_MODE = 0,
    HIRES_MODE = 1,
} display_graphics_mode_t;

typedef enum {
    LM_TEXT_MODE    = 0,
    LM_LORES_MODE   = 1,
    LM_HIRES_MODE   = 2
} line_mode_t;

extern uint32_t lores_color_table[16]; 
extern uint32_t dirty_line[24];
extern line_mode_t line_mode[24];

extern display_mode_t display_mode;
extern display_split_mode_t display_split_mode;
extern display_graphics_mode_t display_graphics_mode;

void force_display_update();
void update_display(cpu_state *cpu);
void free_display();

uint64_t init_display_sdl();
void txt_memory_write(uint16_t , uint8_t );
void update_flash_state(cpu_state *cpu);
void init_device_display();
void render_line(cpu_state *cpu, int y);
