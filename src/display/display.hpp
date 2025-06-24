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
#include "videosystem.hpp"

#define SCALE_X 2
#define SCALE_Y 4
#define BASE_WIDTH 560
#define BASE_HEIGHT 192
#define BORDER_WIDTH 30
#define BORDER_HEIGHT 20

typedef class display_state_t {

public:
    display_state_t();
    ~display_state_t();

    void make_flipped();
    void make_text40_bits();
    void make_hgr_bits();
    void make_lgr_bits();

    SDL_Texture* screenTexture;

    bool flash_state;
    int flash_counter;

    // LUTs mapping video data bytes to video signal bits
    uint8_t flipped[256];
    uint16_t lgr_bits[32];
    uint16_t hgr_bits[256];
    uint16_t text40_bits[256];

    // variable set by newProcessAppleIIFrame functions
    bool kill_color;

    uint8_t *buffer = nullptr;
    EventQueue *event_queue;
    video_system_t *video_system;

} display_state_t;

void update_flash_state(cpu_state *cpu);
void init_mb_device_display(computer_t *computer, SlotType_t slot);

void display_dump_hires_page(cpu_state *cpu, int page);
void display_dump_text_page(cpu_state *cpu, int page);

void display_engine_get_buffer(computer_t *computer, uint8_t *buffer, uint32_t *width, uint32_t *height);