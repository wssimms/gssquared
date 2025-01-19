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
#include <time.h>
/* #include <mach/mach_time.h> */

#include "cpu.hpp"
#include "memory.hpp"

struct cpu_state CPUs[MAX_CPUS];

void cpu_reset(cpu_state *cpu) {
    cpu->pc = read_word(cpu, RESET_VECTOR);
}

uint64_t HZ_RATES[] = {
    0,
    2800000,
    1020500
};

// 1000000000 / cpu->HZ_RATE
uint64_t cycle_durations_ns[] = {
    0, // 357,
    1000000000 / 2800000, // 2.8MHz IIGS,
    (1000000000 / 1020500)+1, // 1020500, Apple II+/IIe - +1 for rounding error, it's actually 979.9
};

/* // cpu->cycle_duration_ns * info.denom / info.numer
uint64_t cycle_durations_ticks[] = {
    8,
    8,
    23
}; */

void set_clock_mode(cpu_state *cpu, clock_mode mode) {
    // Get the conversion factor for mach_absolute_time to nanoseconds
/*     mach_timebase_info_data_t info;
    mach_timebase_info(&info);
 */
    // TODO: if this is ever called from inside a CPU loop, we need to exit that loop
    // immediately in order to avoid weird calculations around.
    // So add a "speedshift" cpu flag.

    cpu->HZ_RATE = HZ_RATES[mode];
    // Lookup time per emulated cycle
    cpu->cycle_duration_ns = cycle_durations_ns[mode];

    cpu->clock_mode = mode;
    fprintf(stdout, "Clock mode: %d HZ_RATE: %llu cycle_duration_ns: %llu \n", cpu->clock_mode, cpu->HZ_RATE, cpu->cycle_duration_ns);
}

/* void set_clock_mode(cpu_state *cpu, clock_mode mode) {
    // Get the conversion factor for mach_absolute_time to nanoseconds
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);

    cpu->HZ_RATE = HZ_RATES[mode];
    // Calculate time per cycle at emulated rate
    cpu->cycle_duration_ns = cycle_durations_ns[mode];
    cpu->cycle_duration_ticks = cycle_durations_ticks[mode];

    // Convert the cycle duration to mach_absolute_time() units
    //cpu->cycle_duration_ticks = (cpu->cycle_duration_ns * info.denom / info.numer); // fudge

    cpu->clock_mode = mode;
    fprintf(stdout, "Clock mode: %d HZ_RATE: %llu cycle_duration_ns: %llu cycle_duration_ticks: %llu\n", cpu->clock_mode, cpu->HZ_RATE, cpu->cycle_duration_ns, cpu->cycle_duration_ticks);
} */

void toggle_clock_mode(cpu_state *cpu) {
    set_clock_mode(cpu, (clock_mode)((cpu->clock_mode + 1) % NUM_CLOCK_MODES));
    fprintf(stdout, "Clock mode: %d HZ_RATE: %llu\n", cpu->clock_mode, cpu->HZ_RATE);
}

processor_model processor_models[NUM_PROCESSOR_TYPES] = {
    { "6502 (nmos)", cpu_6502::execute_next },
    { "65C02 (cmos)", cpu_65c02::execute_next }
};

const char* processor_get_name(int processor_type) {
    return processor_models[processor_type].name;
}