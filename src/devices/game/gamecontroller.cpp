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

/**
 * "Game Controller" or "Game Input"
 * 
 */

#include <SDL2/SDL.h>

#include "cpu.hpp"
#include "bus.hpp"
#include "devices/game/gamecontroller.hpp"

int game_switch_0 = 0;
int game_switch_1 = 0;
int game_switch_2 = 0;


/**
 * this is a relatively naive implementation of game controller,
 * translating mouse position inside the emulator window, to the paddle/joystick
 * inputs. This may not be the easiest thing to control.
 */

/**
 * Each input (start with two) will decay to 0 over time,
 * at which point we flip the high bit of the register on.
 * At each read of -any- inputs, we decrement -all- the input
 * counters.
 * 
 * The counters are measured in cycles.
 * 
 * Read the Mouse X,Y location.
 * 3ms is what the //e reference says the decay time is.
 * 3ms = 3000 cycles.
 */

int game_input_trigger_0 = 0;
int game_input_trigger_1 = 0;
int game_input_trigger_2 = 0;
int game_input_trigger_3 = 0;

uint8_t strobe_game_inputs(cpu_state *cpu, uint16_t address) {
    int mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    game_input_trigger_0 = cpu->cycles + (3000 / 255) * ((mouse_x *255) / WINDOW_WIDTH);
    game_input_trigger_1 = cpu->cycles + (3000 / 255) * ((mouse_y *255) / WINDOW_HEIGHT);
    printf("Strobe game inputs: %d, %d: %d, %d\n", mouse_x, mouse_y, game_input_trigger_0, game_input_trigger_1);
    return 0x00;
}


uint8_t read_game_input_0(cpu_state *cpu, uint16_t address) {
    if (game_input_trigger_0 > cpu->cycles) {
        return 0x80;
    }
    return 0x00;
}

uint8_t read_game_input_1(cpu_state *cpu, uint16_t address) {
    if (game_input_trigger_1 > cpu->cycles) {   
        return 0x80;
    }
    return 0x00;
}

uint8_t read_game_input_2(cpu_state *cpu, uint16_t address) {
    if (game_input_trigger_2 > cpu->cycles) {
        return 0x80;
    }
    return 0x00;
}

uint8_t read_game_input_3(cpu_state *cpu, uint16_t address) {
    if (game_input_trigger_3 > cpu->cycles) {
        return 0x80;
    }
    return 0x00;
}

uint8_t read_game_switch_0(cpu_state *cpu, uint16_t address) {
    game_switch_0 = (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0;
    return game_switch_0 ? 0x80 : 0x00;
}

uint8_t read_game_switch_1(cpu_state *cpu, uint16_t address) {
    game_switch_1 = (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_RIGHT)) != 0;
    return game_switch_1 ? 0x80 : 0x00;
}

uint8_t read_game_switch_2(cpu_state *cpu, uint16_t address) {
    game_switch_2 = (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_MIDDLE)) != 0;
    return game_switch_2 ? 0x80 : 0x00;
}

void init_game_controller(cpu_state *cpu) {

    printf("Initializing game controller\n");
    game_switch_0 = 0;
    game_switch_1 = 0;
    game_switch_2 = 0;

// register the I/O ports
    register_C0xx_memory_read_handler(GAME_ANALOG_0, read_game_input_0);
    register_C0xx_memory_read_handler(GAME_ANALOG_1, read_game_input_1);
    register_C0xx_memory_read_handler(GAME_ANALOG_2, read_game_input_2);
    register_C0xx_memory_read_handler(GAME_ANALOG_3, read_game_input_3);
    register_C0xx_memory_read_handler(GAME_ANALOG_RESET, strobe_game_inputs);
    register_C0xx_memory_read_handler(GAME_SWITCH_0, read_game_switch_0);
    register_C0xx_memory_read_handler(GAME_SWITCH_1, read_game_switch_1);
    register_C0xx_memory_read_handler(GAME_SWITCH_2, read_game_switch_2);
}
