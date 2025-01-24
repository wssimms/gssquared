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

#include <SDL3/SDL.h>

#include "debug.hpp"
#include "cpu.hpp"
#include "devices/keyboard/keyboard.hpp"
#include "display/display.hpp"
#include "devices/game/mousewheel.hpp"

// Base dimensions for aspect ratio calculation
#define WIN_BASE_WIDTH 560
#define WIN_BASE_HEIGHT 384

void handle_window_resize(cpu_state *cpu, int new_w, int new_h) {
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    
    // Calculate the base window size (including borders)
    int base_window_w = (BASE_WIDTH + BORDER_WIDTH*2);
    int base_window_h = (BASE_HEIGHT + BORDER_HEIGHT*2);
    
    // Calculate new scale factors based on window size ratio
    float new_scale_x = (float)new_w / base_window_w;
    float new_scale_y = (float)new_h / base_window_h;
    
    SDL_SetRenderScale(ds->renderer, new_scale_x, new_scale_y);
}

// Loops until there are no events in queue waiting to be read.

void event_poll(cpu_state *cpu) {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_EVENT_QUIT:
                if (DEBUG(DEBUG_GUI)) fprintf(stdout, "quit received, shutting down\n");
                cpu->halt = HLT_USER;
                break;

            case SDL_EVENT_WINDOW_RESIZED: {
                display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
                if (ds && ds->window) {
                    handle_window_resize(cpu, event.window.data1, event.window.data2);
                }
                break;
            }

            case SDL_EVENT_KEY_DOWN:
                handle_sdl_keydown(cpu, event);
                break;

            case SDL_EVENT_MOUSE_BUTTON_DOWN:
                display_capture_mouse(cpu, true);
                //SDL_SetWindowRelativeMouseMode(cpu->window, true);
                break;

            case SDL_EVENT_MOUSE_WHEEL:
                handle_mouse_wheel(cpu, event.wheel.y);
                break;
        }
    }
}