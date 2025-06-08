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

#include <time.h>
/* #include <mach/mach_time.h> */

#include "gs2.hpp"
#include "cpu.hpp"
#include "clock.hpp"

#if 0
// nanosleep isn't real. Combine usleep and the busy wait.
void emulate_clock_cycle_old(cpu_state *cpu) {
    
    /* uint64_t start_time = mach_absolute_time();  // Get the start time */
    uint64_t sleep_time = cpu->cycle_duration_ticks;
    // Optional: Add a small sleep for longer intervals if necessary to reduce CPU usage
    if (sleep_time < 25000) { // disable sleep for now.
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

void emulate_clock_cycle(cpu_state *cpu) {
    
    uint64_t current_time = mach_absolute_time();
    if (cpu->next_tick < current_time) { // clock slip
        cpu->next_tick = current_time + cpu->cycle_duration_ticks;
        cpu->clock_slip++;
    } else {
        uint64_t sleep_time = (cpu->next_tick - current_time);
        if (sleep_time < 25000) { // if we're close, just busy wait
            while ( current_time < cpu->next_tick) {
                current_time = mach_absolute_time();
            }
            cpu->clock_busy++;
        } else {
            struct timespec req = {0, static_cast<long> (sleep_time ) };
            nanosleep(&req, NULL);
            cpu->clock_sleep++;
        }
        cpu->next_tick = cpu->next_tick + cpu->cycle_duration_ticks;
    }
    cpu->last_tick = current_time;
}
#endif

#if 0
void incr_cycles(cpu_state *cpu) {
    cpu->cycles++;
    /* if (cpu->clock_mode != CLOCK_FREE_RUN) { // temp disable this to try different timing approach
        emulate_clock_cycle(cpu);
    } */
}
#endif

#include "platforms.hpp"
#include "display/display.hpp"

void mega_ii_cycle(cpu_state *cpu) {
    cpu->cycles++;
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    
    cpu->horz_counter++;
    if (cpu->horz_counter == 65) {
        cpu->horz_counter = 0;
        cpu->vert_counter++;
        if (cpu->vert_counter == 262) {
            cpu->vert_counter = 0;
        }
    }

    if (cpu->vert_counter >= 34 && cpu->vert_counter < 226) {
        if (cpu->horz_counter >= 25 && cpu->horz_counter < 65) {

            uint8_t vbyte = 0;
            uint16_t vidbits = 0;
            uint32_t line_addr = 0;
            int row = cpu->vert_counter - 34;
            int col = cpu->horz_counter - 25;
            int major = row / 8;
            int minor = row % 8;
            int split_text = (cpu->vert_counter >= 194)
                                && (ds->display_split_mode == SPLIT_SCREEN);

            if (split_text || ds->display_mode == TEXT_MODE) {
                line_addr = ds->display_page_table->text_page_table[major];
                uint8_t ccode = cpu->mmu->read_raw(line_addr + col);
                vbyte = (*cpu->rd->char_rom_data)[8 * ccode + minor];

                // for inverse, xor the pixels with 0xFF to invert them.
                if ((ccode & 0xC0) == 0) {  // inverse
                    vbyte ^= 0xFF;
                } else if (((ccode & 0xC0) == 0x40)) {  // flash
                    vbyte ^= ds->flash_state ? 0xFF : 0;
                }
                for (int count = 7; count; --count) {
                    vidbits = (vidbits << 1) | (vbyte & 1);
                    vidbits = (vidbits << 1) | (vbyte & 1);
                    vbyte >>= 1;
                }
            }
            else if (ds->display_graphics_mode == LORES_MODE) {
                line_addr = ds->display_page_table->text_page_table[major];
                vbyte = cpu->mmu->read_raw(line_addr + col);
                for (int count = 7; count; --count) {
                    vidbits = (vidbits << 1) | (vbyte & 1);
                    vidbits = (vidbits << 1) | (vbyte & 1);
                    vbyte >>= 1;
                }
            }
            else {  // GRAPHICS_MODE && HIRES_MODE
                line_addr = ds->display_page_table->hgr_page_table[major];
                line_addr = line_addr + 0x400 * minor;
                vbyte = cpu->mmu->read_raw(line_addr + col);
                for (int count = 7; count; --count) {
                    vidbits = (vidbits << 1) | (vbyte & 1);
                    vidbits = (vidbits << 1) | (vbyte & 1);
                    vbyte >>= 1;
                }
            }

            cpu->vidbits[row][col] = vidbits;
        }
    }
}