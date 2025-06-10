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

uint8_t flipped[256];
void make_flipped(void) {
    for (int n = 0; n < 256; ++n) {
        uint8_t byte = (uint8_t)n;
        uint8_t flipped_byte = byte >> 7; // leave high bit as high bit
        for (int i = 7; i; --i) {
            flipped_byte = (flipped_byte << 1) | (byte & 1);
            byte >>= 1;
        }
        flipped[n] = flipped_byte;
    }
}

uint16_t hgr_bits[256];
void make_hgr_bits(void) {
    for (uint16_t n = 0; n < 256; ++n) {
        uint8_t  byte = flipped[n];
        uint16_t hgrbits = 0;
        for (int i = 7; i; --i) {
            hgrbits = (hgrbits << 1) | (byte & 1);
            hgrbits = (hgrbits << 1) | (byte & 1);
            byte >>= 1;
        }
        hgrbits = hgrbits << (byte & 1);
        hgr_bits[n] = hgrbits;
        //printf("%4.4x\n", hgrbits);
    }
}

uint16_t lgr_bits[32];
void make_lgr_bits(void) {
    for (int n = 0; n < 16; ++n) {
        uint8_t pattern = flipped[n];
        pattern >>= 3;
        //uint8_t pattern = (uint8_t)n;

        // form even column pattern
        uint16_t lgrbits = 0;
        for (int i = 14; i; --i) {
            lgrbits = (lgrbits << 1) | (pattern & 1);
            pattern = ((pattern & 1) << 3) | (pattern >> 1); // rotate
        }
        lgr_bits[2*n] = lgrbits;

        // form odd column pattern
        lgrbits = 0;
        for (int i = 14; i; --i) {
            lgrbits = (lgrbits << 1) | (pattern & 1);
            pattern = ((pattern & 1) << 3) | (pattern >> 1); // rotate
        }
        lgr_bits[2*n+1] = lgrbits;
    }
}

int made_bits = 0;

void mega_ii_cycle(cpu_state *cpu) {
    if (!made_bits) {
        make_flipped();
        make_hgr_bits();
        make_lgr_bits();
        ++made_bits;
    }

    cpu->cycles++;
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    
    ds->horz_counter++;
    if (ds->horz_counter == 65) {
        ds->horz_counter = 0;
        ds->vert_counter++;
        if (ds->vert_counter == 262) {
            ds->vert_counter = 0;
        }
    }

    ds->video_blanking = 0;
    if (ds->vert_counter >= 34 && ds->vert_counter < 226) {
        ds->video_blanking = 0x80;
        if (ds->horz_counter >= 25 && ds->horz_counter < 65) {

            uint8_t vbyte = 0;
            uint16_t vidbits = 0;
            uint32_t line_addr = 0;
            int row = ds->vert_counter - 34;
            int col = ds->horz_counter - 25;
            int major = row / 8;
            int minor = row % 8;
            int split_text = (ds->vert_counter >= 194)
                                && (ds->display_split_mode == SPLIT_SCREEN);

            if (split_text || ds->display_mode == TEXT_MODE) {
                line_addr = ds->display_page_table->text_page_table[major];
                uint8_t ccode = ds->video_byte = cpu->mmu->read_raw(line_addr + col);
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
                vbyte = ds->video_byte = cpu->mmu->read_raw(line_addr + col);
                vbyte = (vbyte >> (minor & 4)) & 0x0F; // hi or lo nibble
                vbyte = (vbyte << 1) | (col & 1); // different for even/odd columns
                vidbits = lgr_bits[vbyte];
            }
            else {  // GRAPHICS_MODE && HIRES_MODE
                line_addr = ds->display_page_table->hgr_page_table[major];
                line_addr = line_addr + 0x400 * minor;
                vbyte = ds->video_byte = cpu->mmu->read_raw(line_addr + col);
                vidbits = hgr_bits[vbyte];
            }

            ds->vidbits[row][col] = vidbits;
        }
    }
}