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

#include <SDL3/SDL.h>
#include "gs2.hpp"
#include "cpu.hpp"

#define KB_LATCH_ADDRESS 0xC000
#define KB_CLEAR_LATCH_ADDRESS 0xC010

struct keyboard_state_t {
    uint8_t kb_key_strobe = 0x41; 
} ;

/* uint8_t kb_memory_read(uint16_t address);
void kb_memory_write(uint16_t address, uint8_t value); */
void kb_key_pressed(cpu_state *cpu, uint8_t key);
void handle_keydown_iiplus(cpu_state *cpu, const SDL_Event &event);
void init_mb_keyboard(cpu_state *cpu, SlotType_t slot);
