#include <time.h>
#include <mach/mach_time.h>

#include "gs2.hpp"
#include "cpu.hpp"
#include "clock.hpp"

// nanosleep isn't real. Combine usleep and the busy wait.
void emulate_clock_cycle(cpu_state *cpu) {
    
    /* uint64_t start_time = mach_absolute_time();  // Get the start time */
    uint64_t sleep_time = cpu->cycle_duration_ticks;
    // Optional: Add a small sleep for longer intervals if necessary to reduce CPU usage
    if (sleep_time < 25000) {
        uint64_t current_time = mach_absolute_time();
        while (current_time - cpu->last_tick <= cpu->cycle_duration_ticks) {
            current_time = mach_absolute_time();
        }
    } else {
        struct timespec req = {0, static_cast<long> (cpu->last_tick + cpu->cycle_duration_ticks - mach_absolute_time() ) };
        nanosleep(&req, NULL);
    }
    cpu->last_tick = mach_absolute_time();
}

void incr_cycles(cpu_state *cpu) {
    cpu->cycles++;
    if (cpu->clock_mode != CLOCK_FREE_RUN) {
        emulate_clock_cycle(cpu);
    }
}

