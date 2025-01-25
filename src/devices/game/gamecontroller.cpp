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

#include <SDL3/SDL.h>

#include "cpu.hpp"
#include "bus.hpp"
#include "debug.hpp"
#include "devices/game/gamecontroller.hpp"

/* int game_switch_0 = 0;
int game_switch_1 = 0;
int game_switch_2 = 0;
 */

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

/* int game_input_trigger_0 = 0;
int game_input_trigger_1 = 0;
int game_input_trigger_2 = 0;
int game_input_trigger_3 = 0; */

#define GAME_INPUT_DECAY_TIME 2800

uint8_t strobe_game_inputs(cpu_state *cpu, uint16_t address) {
    gamec_state_t *ds = (gamec_state_t *)get_module_state(cpu, MODULE_GAMECONTROLLER);

    float mouse_x, mouse_y;
    SDL_GetMouseState(&mouse_x, &mouse_y);
    if (ds->gtype[0] == GAME_INPUT_TYPE_MOUSE) {
        if (ds->paddle_flip_01) {
            int x_trigger =  cpu->cycles + (3000 / 255) * (255-((mouse_x *255) / WINDOW_WIDTH));
            int y_trigger = cpu->cycles + (3000 / 255) * (255-((mouse_y *255) / WINDOW_HEIGHT));

            ds->game_input_trigger_0 = y_trigger;
            ds->game_input_trigger_1 =x_trigger;   
        } else {
            int x_trigger =  cpu->cycles + (3000 / 255) * ((mouse_x *255) / WINDOW_WIDTH);
            int y_trigger = cpu->cycles + (3000 / 255) * ((mouse_y *255) / WINDOW_HEIGHT);

            ds->game_input_trigger_0 = x_trigger;
            ds->game_input_trigger_1 = y_trigger;
        }
    } else if (ds->gtype[0] == GAME_INPUT_TYPE_MOUSEWHEEL) {
        ds->game_input_trigger_0 = cpu->cycles + (3000 / 255) * ds->mouse_wheel_pos_0;
    } else if (ds->gtype[0] == GAME_INPUT_TYPE_JOYSTICK) {
        // TODO: this gamepad joystick can go horizontally to the full extent, but diagnoally not. can scale
        // the axes larger, to get the corners to full extent if we want.
        int16_t axis0 = SDL_GetJoystickAxis(ds->joystick0, 0);
        int16_t axis1 = SDL_GetJoystickAxis(ds->joystick0, 1);
        
        const float x = (((float)axis0 + 32767.0) / 256.0f);  /* make it -1.0f to 1.0f */
        const float y = (( 32767.0 - (float)axis1) / 256.0f);  /* make it -1.0f to 1.0f */
        int x_trigger =  cpu->cycles + ((GAME_INPUT_DECAY_TIME * x) / 255);
        int y_trigger = cpu->cycles + ((GAME_INPUT_DECAY_TIME * y) / 255);

        ds->game_input_trigger_0 = x_trigger;
        ds->game_input_trigger_1 = y_trigger;
    }
    if (DEBUG(DEBUG_GAME)) fprintf(stdout, "Strobe game inputs: %f, %f: %d, %d\n", mouse_x, mouse_y, ds->game_input_trigger_0, ds->game_input_trigger_1);
    return 0x00;
}

uint8_t read_game_input_0(cpu_state *cpu, uint16_t address) {
    gamec_state_t *ds = (gamec_state_t *)get_module_state(cpu, MODULE_GAMECONTROLLER);

    if (ds->game_input_trigger_0 > cpu->cycles) {
        return 0x80;
    }
    return 0x00;
}

uint8_t read_game_input_1(cpu_state *cpu, uint16_t address) {
    gamec_state_t *ds = (gamec_state_t *)get_module_state(cpu, MODULE_GAMECONTROLLER);
    if (ds->game_input_trigger_1 > cpu->cycles) {   
        return 0x80;
    }
    return 0x00;
}

uint8_t read_game_input_2(cpu_state *cpu, uint16_t address) {
    gamec_state_t *ds = (gamec_state_t *)get_module_state(cpu, MODULE_GAMECONTROLLER);

    if (ds->game_input_trigger_2 > cpu->cycles) {
        return 0x80;
    }
    return 0x00;
}

uint8_t read_game_input_3(cpu_state *cpu, uint16_t address) {
    gamec_state_t *ds = (gamec_state_t *)get_module_state(cpu, MODULE_GAMECONTROLLER);
    if (ds->game_input_trigger_3 > cpu->cycles) {
        return 0x80;
    }
    return 0x00;
}

uint8_t read_game_switch_0(cpu_state *cpu, uint16_t address) {
    gamec_state_t *ds = (gamec_state_t *)get_module_state(cpu, MODULE_GAMECONTROLLER);
    if (ds->gtype[0] == GAME_INPUT_TYPE_JOYSTICK) {
        if (SDL_GetJoystickButton(ds->joystick0, 2) != 0) {
            ds->game_switch_0 = 1;
        } else if (SDL_GetJoystickButton(ds->joystick0, 0) != 0) {
            ds->game_switch_0 = 1;
        } else {
            ds->game_switch_0 = 0;
        }
    } else {
        ds->game_switch_0 = (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_MASK(SDL_BUTTON_LEFT)) != 0;
    }
    return ds->game_switch_0 ? 0x80 : 0x00;
}

uint8_t read_game_switch_1(cpu_state *cpu, uint16_t address) {
    gamec_state_t *ds = (gamec_state_t *)get_module_state(cpu, MODULE_GAMECONTROLLER);
    if (ds->gtype[0] == GAME_INPUT_TYPE_JOYSTICK) {
        if (SDL_GetJoystickButton(ds->joystick0, 3) != 0) {
            ds->game_switch_1 = 1;
        } else if (SDL_GetJoystickButton(ds->joystick0, 1) != 0) {
            ds->game_switch_1 = 1;
        } else {
            ds->game_switch_1 = 0;
        }
    } else {
        ds->game_switch_1 = (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_MASK(SDL_BUTTON_RIGHT)) != 0;
    }
    return ds->game_switch_1 ? 0x80 : 0x00;
}

uint8_t read_game_switch_2(cpu_state *cpu, uint16_t address) {
    gamec_state_t *ds = (gamec_state_t *)get_module_state(cpu, MODULE_GAMECONTROLLER);
    ds->game_switch_2 = (SDL_GetMouseState(NULL, NULL) & SDL_BUTTON_MASK(SDL_BUTTON_MIDDLE)) != 0;
    return ds->game_switch_2 ? 0x80 : 0x00;
}

/**
 * SDL3 handles joysticks / other controllers, by generating "add" events. I.e., 
 * once the controller comes online and connects into the engine, there will be
 * a "add" event.
 * 
 * We can use this to detect when a joystick is connected, and open it.
 * Same for Joystick removal in reverse.
 * 
 * So when a JS comes online, we automatically switch logic to use the joystick;
 * and when it goes offline, we switch back to mouse.
 * TODO: detect a 2nd joystick and wire that into game inputs 2/3 also.
 */
void joystick_added(cpu_state *cpu, SDL_Event *event) {
    gamec_state_t *ds = (gamec_state_t *)get_module_state(cpu, MODULE_GAMECONTROLLER);

     if (ds->joystick0 == NULL) {  /* we don't have a stick yet and one was added, open it! */
        ds->joystick0 = SDL_OpenJoystick(event->jdevice.which);
        if (!ds->joystick0) {
            SDL_Log("Failed to open joystick ID %u: %s", (unsigned int) event->jdevice.which, SDL_GetError());
            return;
        }
        ds->gtype[0] = GAME_INPUT_TYPE_JOYSTICK;
        printf("Opened joystick ID %u\n", (unsigned int) event->jdevice.which);
    }
}

void joystick_removed(cpu_state *cpu, SDL_Event *event) {
    gamec_state_t *ds = (gamec_state_t *)get_module_state(cpu, MODULE_GAMECONTROLLER);
    if (ds->joystick0 && (SDL_GetJoystickID(ds->joystick0) == event->jdevice.which)) {
        SDL_CloseJoystick(ds->joystick0);  /* our joystick was unplugged. */
        ds->joystick0 = NULL;
        ds->gtype[0] = GAME_INPUT_TYPE_MOUSE;
        printf("Closed joystick ID %u\n", (unsigned int) event->jdevice.which);
    }
}

void init_mb_game_controller(cpu_state *cpu) {
    SDL_InitSubSystem(SDL_INIT_JOYSTICK);
    // alloc and init display state
    gamec_state_t *ds = new gamec_state_t;
    ds->game_switch_0 = 0;
    ds->game_switch_1 = 0;
    ds->game_switch_2 = 0;
    ds->game_input_trigger_0 = 0;
    ds->game_input_trigger_1 = 0;
    ds->game_input_trigger_2 = 0;
    ds->game_input_trigger_3 = 0;
    ds->mouse_wheel_pos_0 = 0;
    ds->paddle_flip_01 = 0; // to swap the mouse axes so Y is paddle 0
    ds->gtype[0] = GAME_INPUT_TYPE_MOUSE;
    ds->gtype[1] = GAME_INPUT_TYPE_MOUSE;
    ds->gtype[2] = GAME_INPUT_TYPE_MOUSE;
    ds->gtype[3] = GAME_INPUT_TYPE_MOUSE;
    ds->joystick0 = NULL;
    
    // set in CPU so we can reference later
    set_module_state(cpu, MODULE_GAMECONTROLLER, ds);

    if (DEBUG(DEBUG_GAME)) fprintf(stdout, "Initializing game controller\n");

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
