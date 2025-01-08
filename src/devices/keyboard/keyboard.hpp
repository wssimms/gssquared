#pragma once

#include <SDL2/SDL.h>
#include "../cpu.hpp"

#define KB_LATCH_ADDRESS 0xC000
#define KB_CLEAR_LATCH_ADDRESS 0xC010

/* uint8_t kb_memory_read(uint16_t address);
void kb_memory_write(uint16_t address, uint8_t value); */
void kb_key_pressed(uint8_t key);
void handle_sdl_keydown(cpu_state *cpu, SDL_Event event);
void init_keyboard();
