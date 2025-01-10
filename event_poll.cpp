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

#include <SDL2/SDL.h>

#include "debug.hpp"
#include "cpu.hpp"
#include "devices/keyboard/keyboard.hpp"

// Loops until there are no events in queue waiting to be read.

void event_poll(cpu_state *cpu) {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        switch (event.type) {
            case  SDL_QUIT:
                if (DEBUG(DEBUG_GUI)) fprintf(stdout, "quit received, shutting down\n");
                cpu->halt = HLT_USER;
                break;

            case SDL_KEYDOWN:
                handle_sdl_keydown(cpu, event);
                break;
        }
    }
}