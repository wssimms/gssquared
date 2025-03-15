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

#include "cpu.hpp"

// TODO: get these from the display subsystem. Maybe even query the window.
#define WINDOW_WIDTH 1120
#define WINDOW_HEIGHT 768

#define GAME_STROBE  0xC040
#define GAME_SWITCH_0 0xC061
#define GAME_SWITCH_1 0xC062
#define GAME_SWITCH_2 0xC063
#define GAME_ANALOG_0 0xC064
#define GAME_ANALOG_1 0xC065
#define GAME_ANALOG_2 0xC066
#define GAME_ANALOG_3 0xC067
#define GAME_ANALOG_RESET 0xC070

#define GAME_AN0_OFF 0xC058
#define GAME_AN0_ON  0xC059
#define GAME_AN1_OFF 0xC05A
#define GAME_AN1_ON  0xC05B
#define GAME_AN2_OFF 0xC05C
#define GAME_AN2_ON  0xC05D
#define GAME_AN3_OFF 0xC05E
#define GAME_AN3_ON  0xC05F

enum game_input_type {
    GAME_INPUT_TYPE_MOUSE = 0,
    GAME_INPUT_TYPE_MOUSEWHEEL,
    GAME_INPUT_TYPE_GAMEPAD,
    GAME_INPUT_TYPE_JOYSTICK,
    NUM_GAME_INPUT_TYPES
};

typedef struct gamec_state_t {
    enum game_input_type gtype[4];

    int game_switch_0;
    int game_switch_1;
    int game_switch_2;

    int game_input_trigger_0;
    int game_input_trigger_1;
    int game_input_trigger_2;
    int game_input_trigger_3;

    int mouse_wheel_pos_0; // only one wheel per mouse.
    int paddle_flip_01;
    SDL_Joystick *joystick0;
} gamec_state_t;

void init_mb_game_controller(cpu_state *cpu, uint8_t slot);
void joystick_added(cpu_state *cpu, SDL_Event *event);
void joystick_removed(cpu_state *cpu, SDL_Event *event);
