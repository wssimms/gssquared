#include <cstdio>
#include <time.h>
#include <mach/mach_time.h>

#include "cpu.hpp"
#include "memory.hpp"

struct cpu_state CPUs[MAX_CPUS];

void cpu_reset(cpu_state *cpu) {
    cpu->pc = read_word(cpu, RESET_VECTOR);
}

uint64_t HZ_RATES[] = {
    2800000,
    2800000,
    1024000
};

void set_clock_mode(cpu_state *cpu, clock_mode mode) {
    // Get the conversion factor for mach_absolute_time to nanoseconds
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);

    cpu->HZ_RATE = HZ_RATES[mode];
    // Calculate time per cycle at emulated rate
    cpu->cycle_duration_ns = 1000000000 / cpu->HZ_RATE;

    // Convert the cycle duration to mach_absolute_time() units
    cpu->cycle_duration_ticks = (cpu->cycle_duration_ns * info.denom / info.numer); // fudge

    cpu->clock_mode = mode;
    fprintf(stdout, "Clock mode: %d HZ_RATE: %llu cycle_duration_ns: %llu cycle_duration_ticks: %llu\n", cpu->clock_mode, cpu->HZ_RATE, cpu->cycle_duration_ns, cpu->cycle_duration_ticks);
}

void toggle_clock_mode(cpu_state *cpu) {
    set_clock_mode(cpu, (clock_mode)((cpu->clock_mode + 1) % NUM_CLOCK_MODES));
    fprintf(stdout, "Clock mode: %d HZ_RATE: %llu\n", cpu->clock_mode, cpu->HZ_RATE);
}
