#include <cstdio>
#include <SDL2/SDL.h>
#include "../types.hpp"
#include "../cpu.hpp"
#include "../debug.hpp"
#include "keyboard.hpp"
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

uint8_t kb_memory_read(uint16_t address) {
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

void kb_memory_write(uint16_t address, uint8_t value) {
    if (address == 0xC010) {
        // Clear the keyboard latch
        kb_clear_strobe();
    }
}

void handle_sdl_keydown(cpu_state *cpu, SDL_Event event) {

    // Ignore if only shift is pressed
    uint16_t mod = event.key.keysym.mod;
    SDL_Keycode key = event.key.keysym.sym;

    if (mod & KMOD_SHIFT) {
        if (DEBUG(DEBUG_KEYBOARD)) fprintf(stdout, "shift key pressed: %08X\n", key);
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
        else if (key == SDLK_QUOTE) kb_key_pressed('"');
        else if (key == SDLK_SEMICOLON) kb_key_pressed(':');
        else if (key == SDLK_COMMA) kb_key_pressed('<');
        else if (key == SDLK_PERIOD) kb_key_pressed('>');
        else if (key == SDLK_SLASH) kb_key_pressed('?');
        else kb_key_pressed(key);
        return;
    }

    if (mod & KMOD_CTRL) {
        // Convert lowercase to control code (0x01-0x1A)
        if (key >= 'a' && key <= 'z') {
            key = key - 'a' + 1;
            kb_key_pressed(key);
            if (DEBUG(DEBUG_KEYBOARD)) fprintf(stdout, "control key pressed: %08X\n", key);
        }
    } 
    else {
        // convert lowercase characters to uppercase for Apple ][+
        if (key == SDLK_F12) { cpu->halt = HLT_USER; return; }
        if (key == SDLK_LEFT) { kb_key_pressed(0x08); return; }
        if (key == SDLK_RIGHT) { kb_key_pressed(0x15); return; }
        if (key >= 'a' && key <= 'z') key = key - 'a' + 'A';

        kb_key_pressed(key);
    }
    if (DEBUG(DEBUG_KEYBOARD)) fprintf(stdout, "key pressed: %08X\n", key);
}
