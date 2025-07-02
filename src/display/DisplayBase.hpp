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

#include "Device_ID.hpp"
#include "SDL3/SDL_render.h"
#include "cpu.hpp"
#include "devices/displaypp/RGBA.hpp"

struct video_system_t;

#define SCALE_X 2
#define SCALE_Y 4
#define BASE_WIDTH 567
#define BASE_HEIGHT 192
#define BORDER_WIDTH 30
#define BORDER_HEIGHT 20

class Display
{
protected:
    int width;
    int height;
    int flash_counter;

    // LUTs mapping video data bytes to video signal bits
    uint8_t flipped[256];
    uint16_t lgr_bits[32];
    uint16_t hgr_bits[256];
    uint16_t text40_bits[256];

    RGBA_t *output = nullptr;
    RGBA_t *buffer = nullptr;
    computer_t *computer = nullptr;
    EventQueue *event_queue = nullptr;
    video_system_t *video_system = nullptr;

    SDL_Texture* screenTexture;

    void make_flipped();
    void make_text40_bits();
    void make_hgr_bits();
    void make_lgr_bits();

public:
    Display(computer_t *computer, int height, int width);
    ~Display();

    virtual bool update_display(cpu_state *cpu);
    void register_display_device(computer_t *computer, device_id id);

    inline uint8_t flash_mask() { return (flash_counter >= 15) ? 0xFF : 0; }
    inline int get_width() { return width; }
    inline int get_height() { return height; }

    inline EventQueue * get_event_queue() { return event_queue; }
    inline SDL_Texture * get_texture() { return screenTexture; }

    void get_buffer(uint8_t    *buffer,
                    uint32_t   *width,
                    uint32_t   *height);
};

void display_dump_hires_page(cpu_state *cpu, int page);
void display_dump_text_page(cpu_state *cpu, int page);
bool handle_display_event(Display *ds, const SDL_Event &event);
