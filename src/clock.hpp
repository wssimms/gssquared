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

#include "cpu.hpp"

struct cpu_state; // forward declaration

typedef enum {
    CLOCK_FREE_RUN = 0,
    CLOCK_1_024MHZ,
    CLOCK_2_8MHZ,
    CLOCK_4MHZ,
    NUM_CLOCK_MODES
} clock_mode_t;

typedef struct {
    double hz_rate;
    double cycle_duration_ns;
    uint64_t cycles_per_burst;
} clock_mode_info_t;

extern clock_mode_info_t clock_mode_info[NUM_CLOCK_MODES];

void emulate_clock_cycle(cpu_state *cpu) ;

//#define incr_cycles(cpu) cpu->cycles++;

void incr_cycles(cpu_state *cpu);
void rgb_video_cycle (cpu_state *cpu);
void mono_video_cycle (cpu_state *cpu);
void ntsc_video_cycle (cpu_state *cpu);
