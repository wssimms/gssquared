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

#include "gs2.hpp"
#include "cpu.hpp"

#define MB_6522_DDRA 0x03
#define MB_6522_DDRB 0x02
#define MB_6522_ORA  0x01
#define MB_6522_ORB  0x00

#define MB_6522_1 0x00
#define MB_6522_2 0x80

struct mb_6522_data {
    uint8_t ora;
    uint8_t orb;
    uint8_t ddra;
    uint8_t ddrb; 
    uint8_t reg_num;   
};

class MockingboardEmulator; // forward declaration

struct mb_cpu_data {
    MockingboardEmulator *mockingboard;
    mb_6522_data d_6522[2];
    std::vector<float> audio_buffer;
    SDL_AudioStream *stream;
    uint64_t last_cycle;
};

void init_slot_mockingboard(cpu_state *cpu, SlotType_t slot);
void mb_write_Cx00(cpu_state *cpu, uint16_t addr, uint8_t data);
uint8_t mb_read_Cx00(cpu_state *cpu, uint16_t addr);
void generate_mockingboard_frame(cpu_state *cpu);
