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

#pragma once

#include "gs2.hpp"
#include "cpu.hpp"
#include "platforms.hpp"
#include "videosystem.hpp"

#define SCALE_X 2
#define SCALE_Y 4
#define BASE_WIDTH 560
#define BASE_HEIGHT 192
#define BORDER_WIDTH 30
#define BORDER_HEIGHT 20

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

/** Display Modes */
/* 
typedef enum {
    DM_ENGINE_NTSC = 0,
    DM_ENGINE_RGB,
    DM_ENGINE_MONO,
    DM_NUM_COLOR_ENGINES
} display_color_engine_t;

typedef enum {
    DM_MONO_WHITE = 0,
    DM_MONO_GREEN,
    DM_MONO_AMBER,
    DM_NUM_MONO_MODES
} display_mono_color_t;

typedef enum {
    DM_PIXEL_FUZZ = 0,
    DM_PIXEL_SQUARE,
    DM_NUM_PIXEL_MODES
} display_pixel_mode_t; */

/** End Display Modes */

typedef uint16_t display_page_table_t[24] ;

typedef struct display_page_t {
    uint16_t text_page_start;
    uint16_t text_page_end;
    display_page_table_t text_page_table;
    uint16_t hgr_page_start;
    uint16_t hgr_page_end;
    display_page_table_t hgr_page_table;
} display_page_t;

typedef enum {
    DISPLAY_PAGE_1 = 0,
    DISPLAY_PAGE_2,
    NUM_DISPLAY_PAGES
} display_page_number_t;

typedef class display_state_t {

public:
    display_state_t();
    ~display_state_t();

    void make_flipped();
    void make_text40_bits();
    void make_hgr_bits();
    void make_lgr_bits();

    SDL_Texture* screenTexture;

    /*
    display_color_engine_t display_color_engine;
    display_mono_color_t display_mono_color;
    display_pixel_mode_t display_pixel_mode;
    */

    display_mode_t display_mode;
    display_split_mode_t display_split_mode;
    display_graphics_mode_t display_graphics_mode;
    display_page_number_t display_page_num;

    bool flash_state;
    int flash_counter;

    // LUTs mapping video data bytes to video signal bits
    uint8_t flipped[256];
    uint16_t lgr_bits[32];
    uint16_t hgr_bits[256];
    uint16_t text40_bits[256];

    // variables set by ntsc_video_cycle()
    bool kill_color;

    uint8_t *buffer = nullptr;
    EventQueue *event_queue;
    video_system_t *video_system;

} display_state_t;


extern RGBA_t lores_color_table[16]; 

/* void update_display(cpu_state *cpu); */

void txt_memory_write(uint16_t , uint8_t );
void update_flash_state(cpu_state *cpu);
void init_mb_device_display(computer_t *computer, SlotType_t slot);
void render_line_ntsc(cpu_state *cpu, int y);
void render_line_rgb(cpu_state *cpu, int y);
void render_line_mono(cpu_state *cpu, int y);
void pre_calculate_font(rom_data *rd);
void init_display_font(rom_data *rd);

void display_dump_hires_page(cpu_state *cpu, int page);
void display_dump_text_page(cpu_state *cpu, int page);

void display_engine_get_buffer(computer_t *computer, uint8_t *buffer, uint32_t *width, uint32_t *height);