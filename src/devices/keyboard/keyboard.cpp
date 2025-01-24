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

#include <cstdio>
#include <SDL3/SDL.h>
#include "gs2.hpp"
#include "cpu.hpp"
#include "debug.hpp"
#include "keyboard.hpp"
#include "bus.hpp"
#include "devices/speaker/speaker.hpp"
#include "devices/loader.hpp"
#include "display/display.hpp"
// Software should be able to:
// Read keyboard from register at $C000.
// Write to the keyboard clear latch at $C010.

uint8_t kb_key_strobe = 0xC1;

void kb_key_pressed(uint8_t key) {
    kb_key_strobe = key | 0x80;
}

void kb_clear_strobe() {
    kb_key_strobe = kb_key_strobe & 0x7F;
}

uint8_t kb_memory_read(cpu_state *cpu, uint16_t address) {
    //fprintf(stderr, "kb_memory_read %04X\n", address);
    if (address == 0xC000) {
        uint8_t key = kb_key_strobe;
        return key;
    }
    if (address == 0xC010) {
        // Clear the keyboard latch
        kb_clear_strobe();
        return 0xEE;
    }
    return 0xEE;
}

void kb_memory_write(cpu_state *cpu, uint16_t address, uint8_t value) {
    if (address == 0xC010) {
        // Clear the keyboard latch
        kb_clear_strobe();
    }
}

void decode_key_mod(SDL_Keycode key, SDL_Keymod mod) {
    fprintf(stdout, "key: %08X mod: ", key);
    if (mod & SDL_KMOD_LSHIFT) {
        fprintf(stdout, "LSHIFT ");
    }
    if (mod & SDL_KMOD_RSHIFT) {
        fprintf(stdout, "RSHIFT ");
    }
    if (mod & SDL_KMOD_LCTRL) {
        fprintf(stdout, "LCTRL ");
    }
    if (mod & SDL_KMOD_RCTRL) {
        fprintf(stdout, "RCTRL ");
    }
    if (mod & SDL_KMOD_LALT) {
        fprintf(stdout, "LALT ");
    }
    if (mod & SDL_KMOD_RALT) {
        fprintf(stdout, "RALT ");
    }
    fprintf(stdout, "\n");
}

void handle_sdl_keydown(cpu_state *cpu, SDL_Event event) {

    // Ignore if only shift is pressed
    /* uint16_t mod = event.key.keysym.mod;
    SDL_Keycode key = event.key.keysym.sym; */
    SDL_Keymod mod = event.key.mod;
    SDL_Keycode key = event.key.key;

    if (DEBUG(DEBUG_KEYBOARD))  decode_key_mod(key, mod);

    if (mod & SDL_KMOD_SHIFT) {
        /* if (DEBUG(DEBUG_KEYBOARD)) fprintf(stdout, "shift key pressed: %08X\n", key); */
        if ((key == SDLK_LSHIFT) || (key == SDLK_RSHIFT)) return;
        if (key == SDLK_EQUALS) kb_key_pressed('+');
        else if (key == SDLK_MINUS) kb_key_pressed('_');
        else if (key == SDLK_1) kb_key_pressed('!');
        else if (key == SDLK_2) kb_key_pressed('@');
        else if (key == SDLK_3) kb_key_pressed('#');
        else if (key == SDLK_4) kb_key_pressed('$');
        else if (key == SDLK_5) kb_key_pressed('%');
        else if (key == SDLK_6) kb_key_pressed('^');
        else if (key == SDLK_7) kb_key_pressed('&');
        else if (key == SDLK_8) kb_key_pressed('*');
        else if (key == SDLK_9) kb_key_pressed('(');
        else if (key == SDLK_0) kb_key_pressed(')');
        else if (key == SDLK_APOSTROPHE) kb_key_pressed('"');
        else if (key == SDLK_SEMICOLON) kb_key_pressed(':');
        else if (key == SDLK_COMMA) kb_key_pressed('<');
        else if (key == SDLK_PERIOD) kb_key_pressed('>');
        else if (key == SDLK_SLASH) kb_key_pressed('?');
        else kb_key_pressed(key);
        return;
    }

    if (mod & SDL_KMOD_CTRL) {
        // Convert lowercase to control code (0x01-0x1A)
        if (key >= 'a' && key <= 'z') {
            key = key - 'a' + 1;
            kb_key_pressed(key);
            /* if (DEBUG(DEBUG_KEYBOARD)) fprintf(stdout, "control key pressed: %08X\n", key); */
        } else if (key == SDLK_F10) { reset_system(cpu); return; }
    } 
    else {
        // convert lowercase characters to uppercase for Apple ][+
        if (key == SDLK_F12) { cpu->halt = HLT_USER; return; }
        if (key == SDLK_F9) { 
            toggle_clock_mode(cpu);
            return; 
        }
        if (key == SDLK_F8) {
            toggle_speaker_recording(cpu);
            return;
        }
        if (key == SDLK_F7) {
            loader_execute(cpu);
            return;
        }
         if (key == SDLK_F3) {
            toggle_display_fullscreen(cpu);
            // TODO: maybe also set the sharp scale mode.
            return;
        }
        if (key == SDLK_F2) {
            toggle_display_color_mode(cpu);
            force_display_update(cpu);
            // TODO: maybe also set the sharp scale mode.
            return;
        }
        if (key == SDLK_F1) {
            display_capture_mouse(cpu, false);
            //SDL_SetWindowRelativeMouseMode(cpu->window, false);
            return;
        }
        if (key == SDLK_LEFT) { kb_key_pressed(0x08); return; }
        if (key == SDLK_RIGHT) { kb_key_pressed(0x15); return; }
        if (key >= 'a' && key <= 'z') key = key - 'a' + 'A';

        kb_key_pressed(key);
    }
    /* if (DEBUG(DEBUG_KEYBOARD)) fprintf(stdout, "key pressed: %08X\n", key); */
}

void init_mb_keyboard(cpu_state *cpu) {
    if (DEBUG(DEBUG_KEYBOARD)) fprintf(stdout, "init_keyboard\n");
    register_C0xx_memory_read_handler(0xC000, kb_memory_read);
    register_C0xx_memory_read_handler(0xC010, kb_memory_read);
    register_C0xx_memory_write_handler(0xC010, kb_memory_write);
}
