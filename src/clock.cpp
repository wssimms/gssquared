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

uint16_t hgr_vcount_addr[262] = {
    0x3278, 0x3678, 0x3A78, 0x3E78,
    // upper border
    0x22F8, 0x26F8, 0x2AF8, 0x2EF8, 0x32F8, 0x36F8, 0x3AF8, 0x3EF8,
    0x2378, 0x2778, 0x2B78, 0x2F78, 0x3378, 0x3778, 0x3B78, 0x3F78,
    0x23F8, 0x27F8, 0x2BF8, 0x2FF8, 0x33F8, 0x37F8, 0x3BF8, 0x3FF8,
    0x2BF8, 0x2FF8, 0x33F8, 0x37F8, 0x3BF8, 0x3FF8,

    0x2000, 0x2400, 0x2800, 0x2C00, 0x3000, 0x3400, 0x3800, 0x3C00, //
    0x2080, 0x2480, 0x2880, 0x2C80, 0x3080, 0x3480, 0x3880, 0x3C80, //
    0x2100, 0x2500, 0x2900, 0x2D00, 0x3100, 0x3500, 0x3900, 0x3D00, //
    0x2180, 0x2580, 0x2980, 0x2D80, 0x3180, 0x3580, 0x3980, 0x3D80, //
    0x2200, 0x2600, 0x2A00, 0x2E00, 0x3200, 0x3600, 0x3A00, 0x3E00, //
    0x2280, 0x2680, 0x2A80, 0x2E80, 0x3280, 0x3680, 0x3A80, 0x3E80, //
    0x2300, 0x2700, 0x2B00, 0x2F00, 0x3300, 0x3700, 0x3B00, 0x3F00, //
    0x2380, 0x2780, 0x2B80, 0x2F80, 0x3380, 0x3780, 0x3B80, 0x3F80, //

    0x2028, 0x2428, 0x2828, 0x2C28, 0x3028, 0x3428, 0x3828, 0x3C28, //
    0x20A8, 0x24A8, 0x28A8, 0x2CA8, 0x30A8, 0x34A8, 0x38A8, 0x3CA8, //
    0x2128, 0x2528, 0x2928, 0x2D28, 0x3128, 0x3528, 0x3928, 0x3D28, //
    0x21A8, 0x25A8, 0x29A8, 0x2DA8, 0x31A8, 0x35A8, 0x39A8, 0x3DA8, //
    0x2228, 0x2628, 0x2A28, 0x2E28, 0x3228, 0x3628, 0x3A28, 0x3E28, //
    0x22A8, 0x26A8, 0x2AA8, 0x2EA8, 0x32A8, 0x36A8, 0x3AA8, 0x3EA8, //
    0x2328, 0x2728, 0x2B28, 0x2F28, 0x3328, 0x3728, 0x3B28, 0x3F28, //
    0x23A8, 0x27A8, 0x2BA8, 0x2FA8, 0x33A8, 0x37A8, 0x3BA8, 0x3FA8, //

    0x2050, 0x2450, 0x2850, 0x2C50, 0x3050, 0x3450, 0x3850, 0x3C50,
    0x20D0, 0x24D0, 0x28D0, 0x2CD0, 0x30D0, 0x34D0, 0x38D0, 0x3CD0,
    0x2150, 0x2550, 0x2950, 0x2D50, 0x3150, 0x3550, 0x3950, 0x3D50,
    0x21D0, 0x25D0, 0x29D0, 0x2DD0, 0x31D0, 0x35D0, 0x39D0, 0x3DD0,
    0x2250, 0x2650, 0x2A50, 0x2E50, 0x3250, 0x3650, 0x3A50, 0x3E50,
    0x22D0, 0x26D0, 0x2AD0, 0x2ED0, 0x32D0, 0x36D0, 0x3AD0, 0x3ED0,
    0x2350, 0x2750, 0x2B50, 0x2F50, 0x3350, 0x3750, 0x3B50, 0x3F50,
    0x23D0, 0x27D0, 0x2BD0, 0x2FD0, 0x33D0, 0x37D0, 0x3BD0, 0x3FD0,
    // lower border
    0x2078, 0x2478, 0x2878, 0x2C78, 0x3078, 0x3478, 0x3878, 0x3C78,
    0x20F8, 0x24F8, 0x28F8, 0x2CF8, 0x30F8, 0x34F8, 0x38F8, 0x3CF8,
    0x2178, 0x2578, 0x2978, 0x2D78, 0x3178, 0x3578, 0x3978, 0x3D78,
    0x21F8, 0x25F8, 0x29F8, 0x2DF8, 0x31F8, 0x35F8, 0x39F8, 0x3DF8,
    // vsync
    0x2278, 0x2678, 0x2A78, 0x2E78
};

uint16_t txt_vcount_addr[262] = {
    0x0678, 0x0678, 0x0678, 0x0678,
    // upper border
    0x06F8, 0x06F8, 0x06F8, 0x06F8, 0x06F8, 0x06F8, 0x06F8, 0x08F8,
    0x0778, 0x0778, 0x0778, 0x0778, 0x0778, 0x0778, 0x0778, 0x0778,
    0x07F8, 0x07F8, 0x07F8, 0x07F8, 0x07F8, 0x07F8, 0x07F8, 0x07F8,
    0x07F8, 0x07F8, 0x07F8, 0x07F8, 0x07F8, 0x07F8,

    0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400, 0x0400,
    0x0480, 0x0480, 0x0480, 0x0480, 0x0480, 0x0480, 0x0480, 0x0480,
    0x0500, 0x0500, 0x0500, 0x0500, 0x0500, 0x0500, 0x0500, 0x0500,
    0x0580, 0x0580, 0x0580, 0x0580, 0x0580, 0x0580, 0x0580, 0x0580,
    0x0600, 0x0600, 0x0600, 0x0600, 0x0600, 0x0600, 0x0600, 0x0600,
    0x0680, 0x0680, 0x0680, 0x0680, 0x0680, 0x0680, 0x0680, 0x0680,
    0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700, 0x0700,
    0x0780, 0x0780, 0x0780, 0x0780, 0x0780, 0x0780, 0x0780, 0x0780,

    0x0428, 0x0428, 0x0428, 0x0428, 0x0428, 0x0428, 0x0428, 0x0428,
    0x04A8, 0x04A8, 0x04A8, 0x04A8, 0x04A8, 0x04A8, 0x04A8, 0x04A8,
    0x0528, 0x0528, 0x0528, 0x0528, 0x0528, 0x0528, 0x0528, 0x0528,
    0x05A8, 0x05A8, 0x05A8, 0x05A8, 0x05A8, 0x05A8, 0x05A8, 0x05A8,
    0x0628, 0x0628, 0x0628, 0x0628, 0x0628, 0x0628, 0x0628, 0x0628,
    0x06A8, 0x06A8, 0x06A8, 0x06A8, 0x06A8, 0x06A8, 0x06A8, 0x06A8,
    0x0728, 0x0728, 0x0728, 0x0728, 0x0728, 0x0728, 0x0728, 0x0728,
    0x07A8, 0x07A8, 0x07A8, 0x07A8, 0x07A8, 0x07A8, 0x07A8, 0x07A8,

    0x0450, 0x0450, 0x0450, 0x0450, 0x0450, 0x0450, 0x0450, 0x0450,
    0x04D0, 0x04D0, 0x04D0, 0x04D0, 0x04D0, 0x04D0, 0x04D0, 0x04D0,
    0x0550, 0x0550, 0x0550, 0x0550, 0x0550, 0x0550, 0x0550, 0x0550,
    0x05D0, 0x05D0, 0x05D0, 0x05D0, 0x05D0, 0x05D0, 0x05D0, 0x05D0,
    0x0650, 0x0650, 0x0650, 0x0650, 0x0650, 0x0650, 0x0650, 0x0650,
    0x06D0, 0x06D0, 0x06D0, 0x06D0, 0x06D0, 0x06D0, 0x06D0, 0x06D0,
    0x0750, 0x0750, 0x0750, 0x0750, 0x0750, 0x0750, 0x0750, 0x0750,
    0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x07D0, 0x07D0,
    // lower border
    0x0478, 0x0478, 0x0478, 0x0478, 0x0478, 0x0478, 0x0478, 0x0478,
    0x04F8, 0x04F8, 0x04F8, 0x04F8, 0x04F8, 0x04F8, 0x04F8, 0x04F8,
    0x0578, 0x0578, 0x0578, 0x0578, 0x0578, 0x0578, 0x0578, 0x0578,
    0x05F8, 0x05F8, 0x05F8, 0x05F8, 0x05F8, 0x05F8, 0x05F8, 0x05F8,
    // vsync
    0x0678, 0x0678, 0x0678, 0x0678
};

uint16_t horz_count_offs[65] = {
    // HBL
     0,  0,  1,  2,  3,
     4,  5,  6,  7,  8,
     9, 10, 11, 12, 13,
    14, 15, 16, 17, 18,
    19, 20, 21, 22, 23,
    // Video
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    30, 31, 32, 33, 34, 35, 36, 37, 38, 39,
};

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
        uint16_t evnbits = 0;
        for (int i = 14; i; --i) {
            evnbits = (evnbits << 1) | (pattern & 1);
            pattern = ((pattern & 1) << 3) | (pattern >> 1); // rotate
        }

        // form odd column pattern
        uint16_t oddbits = 0;
        for (int i = 14; i; --i) {
            oddbits = (oddbits << 1) | (pattern & 1);
            pattern = ((pattern & 1) << 3) | (pattern >> 1); // rotate
        }

        lgr_bits[2*n]   = ((evnbits >> 2) | (oddbits << 12)) & 0x3FFF;
        lgr_bits[2*n+1] = ((oddbits >> 2) | (evnbits << 12)) & 0x3FFF;
    }
}

/*
 * Here's what's going on in the function mega_ii_cycle()
 *
 *     In the Apple II, the display is generated by the video subsystem.
 * Central to the video subsytem are two counters, the horizontal counter
 * and the vertical counter. The horizontal counter is 7 bits and can be
 * in one of 65 states, with values 0x00, then 0x40-0x7F. Of these 65
 * possible states, values 0x58-0x7F represent the 40 1-Mhz clock periods
 * in which video data is displayed on a scan line. Horizontal blanking and
 * horizontal sync occur during the other 25 states (0x00,0x40-0x57).
 *     The vertical counter is 9 bits and can be in one of 262 states, with
 * values 0xFA-0x1FF. Of these 262 possible states, values 0x100-0x1BF
 * represent the 192 scan lines in which video data is displayed on screen.
 * Vertical blanking and vertical sync occur during the other 70 states
 * (0x1C0-0x1FF,0xFA-0xFF).
 *     The horizontal counter is updated to the next state every 1-Mhz cycle.
 * If it is in states 0x40-0x7F it is incremented. Incrementing the horizontal
 * counter from 0x7F wraps it around to 0x00. If the horizontal counter is in
 * state 0x00, it is updated by being set to 0x40 instead of being incremented.
 *     The vertical counter is updated to the next state every time the
 * horizontal counter wraps around to 0x00. If it is in states 0xFA-0x1FF,
 * it is incremented. If the vertical counter is in state 0x1FF, it is updated
 * by being set to 0xFA instead of being incremented.
 *     The bits of the counters can be labaled as follows:
 * 
 * horizontal counter:        H6 H5 H4 H3 H2 H1 H0
 *   vertical counter:  V5 V4 V3 V2 V1 V0 VC VB VA
 *      most significant ^                       ^ least significant
 * 
 *     During each 1-Mhz cycles, an address is formed as a logical combination
 * of these bits and terms constructed from the values of the soft switches
 * TEXT/GRAPHICS, LORES/HIRES, PAGE1/PAGE2, MIXED/NOTMIXED. The video subsytem
 * reads the byte at this address from RAM and, if the counters are in states
 * that correspond to video display times, use the byte to display video on
 * screen. How this byte affects the video display depends on the current video
 * mode as set by the soft switches listed above. If the counters are in states
 * that correspond to horizontal or vertical blanking times, a byte is still
 * read from the address formed by the video subsystem but it has no effect
 * on the display. However, the byte most recently read by the video subsystem,
 * whether it affects the video display or not, can be obtained by a program
 * by reading an address that does not result in data being driven onto the
 * data bus. That is, by reading an address that does not correspond to any
 * RAM/ROM or peripheral register.
 *     The address read by the video subsystem in each cycle consists of 16
 * bits labeled A15 (most significant) down to A0 (least significant) and is
 * formed as described below:
 * 
 * The least signficant 3 bits are just the least signifcant 3 bits of the
 * horizontal counter:
 * 
 *     A0 = H0, A1 = H1, A2 = H2
 * 
 * The next 4 bits are formed by an arithmetic sum involving bits from the
 * horizontal and vertical counters:
 *
 *       1  1  0  1
 *         H5 H4 H3
 *   +  V4 V3 V4 V3
 *   --------------
 *      A6 A5 A4 A3
 * 
 * The next 3 bits are just bits V0-V2 of the vertical counter (nb: these
 * are NOT the least signficant bits of the vertical counter).
 * 
 *     A7 = V0, A8 = V1, A9 = V2
 * 
 * The remaining bits differ depending on whether hires graphics has been
 * selected or not and whether, if hires graphics has been selected, mixed
 * mode is on or off, and whether, if mixed mode is on, the vertical counter
 * currently corresponds to a scanline in which text is to be displayed.
 * 
 * If hires graphics IS NOT currently to be displayed, then
 * 
 *     A10 = PAGE1  : 1 if page one is selected, 0 if page two is selected
 *     A11 = PAGE2  : 0 if page one is selected, 1 if page two is selected
 *     A12 = HBL    : 1 if in horizontal blanking time (horizontal counter
 *                    states 0x00,0x40-0x57), 0 if not in horizontal blanking
 *                    time (horizontal counter states 0x58-0x7F)
 *     A13 = 0
 *     A14 = 0
 *     A15 = 0
 * 
 *  If hires graphics IS currently to be displayed, then
 *
 *     A10 = VA     : the least significant bit of the vertical counter
 *     A11 = VB
 *     A12 = VC
 *     A13 = PAGE1  : 1 if page one is selected, 0 if page two is selected
 *     A14 = PAGE2  : 0 if page one is selected, 1 if page two is selected
 *     A15 = 0
 * 
 * The code in mega_ii_cycle implements the horizontal and vertical counters
 * and the formation of the video scan address as described above in a fairly
 * straightforward way. The main complication is how mixed mode is handled.
 * When hires mixed mode is set on the Apple II, the product V2 & V4 deselects
 * hires mode. V2 & V4 is true (1) during vertical counter states 0x1A0-0x1BF
 * and 0x1E0-0x1FF, corresponding to scanlines 160-191 (the text window at
 * the bottom of the screen in mixed mode) and scanlines 224-261 (undisplayed
 * lines in vertical blanking time). This affects how bits A14-A10 of the
 * address is formed.
 * 
 * These bits are formed in the following way: Three variables (data members)
 * called page_bit, lores_mode_mask and hires_mode_mask are established which
 * store the current video settings. If page one is selected, page_bit = 0x2000.
 * If page two is selected, page_bit = 0x4000, which in both cases places a set
 * bit at the correct location for forming the address when displaying hires
 * graphics. If hires graphics is selected, hires_mode_mask = 0x7C00, with the
 * set bits corresponding to A14-A10, and lores_mode_mask = 0x0000. If hires
 * graphics is not selected, hires_mode_mask = 0x0000, and lores_mode_mask =
 * 0x1C00, with the set bits corresponding to A12-A10. Thus, these two variables
 * are masks for the high bits of the address in each mode.
 * 
 * For each cycle, a variable HBL is given the value 0x1000 if the horizontal
 * count < 0x58, and the value 0x0000 if the horizontal count >= 0x58, then
 * values for bits A14-A10 are formed for both the hires and not-hires cases.
 * If there were no mixed mode, the values would be formed like this:
 * 
 *     Hires A15-A10 = (page_bit | (vertical-count << 10)) & hires_mode_bits
 *     Lores A15-A10 = ((page_bit >> 3) | HBL) & lores_mode_bits
 * 
 * Since the variables hires_mode_bits and lores_mode_bits are masks, they
 * ensure that at all times, one of these values is zero, and the other is the
 * correct high bits of the address scanned by the video system.
 * 
 * To implement mixed mode, two more variables are introduced: mixed_lores_mask
 * and mixed_hires_mask. If mixed mode is not selected, or if mixed mode is
 * selected but V2 & V4 == 0, then mixed_lores_mask = 0x0000 and mixed_hires_mask
 * = 0x7C00. If mixed mode is selected and (V2 & V4) == 1, then mixed_lores_mask
 * = 0x1C00 and mixed_hires_mask = 0x0000.
 * 
 * Combined masks for the high bits of both text/lores and hires are then formed
 * 
 *     combined_lores_mask = mixed_lores_mask | lores_mode_mask
 *     combined_hires_mask = mixed_hires_mask & hires_mode_mask
 * 
 * This indicates that a text/lores address will be generated if either
 * text/lores mode is selected (lores_mode_mask != 0) OR if hires mixed mode
 * is selected and the vertical counter specifies a text mode scanline
 * (mixed_lores_mask != 0). A hires address will be generated if hires mode is
 * selected (hires_mode_mask != 0) AND the vertical counter does not specify a
 * text mode scanline (mixed_hires_mask != 0).
 * 
 * The high bits of the address generated by the video scanner for both
 * text/lores and hires mode are then formed
 * 
 *     Hires A15-A10 = (page_bit | (vertical-count << 10)) & combined_hires_mask
 *     Lores A15-A10 = ((page_bit >> 3) | HBL) & combined_lores_mask
 * 
 * The result is that, for each cycle, either Hires A15-A10 is nonzero or
 * Lores A15-A10 is nonzero, depending on the selected graphics mode and the
 * state of the vertical counter.
 *
 * Finally the address to be scanned is formed by summing the individual parts
 * A2-A0, A6-A3, A9-A7, A15-A10.
 */

int made_bits = 0;
int text_cycles = 0;

void mega_ii_cycle(cpu_state *cpu)
{
    if (!made_bits) {
        // lazy initialization
        make_flipped();
        make_hgr_bits();
        make_lgr_bits();
        ++made_bits;
    }

    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);

    cpu->cycles++;

    if (ds->hcount) {
        ds->hcount = (ds->hcount + 1) & 0x7F;
        if (ds->hcount == 0) {
            ds->vcount = (ds->vcount + 1) & 0x1FF;
            if (ds->vcount == 0)
                ds->vcount = 0xFA;
        }
    }
    else {
        ds->hcount = 0x40;
    }

    // A2-A0 = H2-H0
    uint32_t A2toA0 = ds->hcount & 7;

    // A6-A3
    uint32_t V3V4V3V4 = ((ds->vcount & 0xC0) >> 1) | ((ds->vcount & 0xC0) >> 3);
    uint32_t A6toA3 = (0x68 + (ds->hcount & 0x38) + V3V4V3V4) & 0x78;

    // A9-A7 = V2-V0
    uint32_t A9toA7 = (ds->vcount & 0x38) << 4;

    // build masks for mixed mode text/lores vs. hires
    uint32_t mixed_lores_mask = 0x1C00;
    uint32_t mixed_hires_mask = 0;
    if (ds->display_split_mode == FULL_SCREEN || (ds->vcount & 0xA0) != 0xA0)
    {
        mixed_lores_mask = 0;
        mixed_hires_mask = 0x7C00;
    }

    // text/lores addresses if text/lores mode OR mixed mode text scanlines
    uint32_t combined_lores_mask = mixed_lores_mask | ds->lores_mode_mask;
    // hires addresses if hires mode AND mixed mode hires scanlines
    uint32_t combined_hires_mask = mixed_hires_mask & ds->hires_mode_mask;

    // A15-A10
    uint32_t HBL = (ds->hcount < 0x58) << 12;
    uint32_t VCVBVA = ds->vcount & 7;
    uint32_t LoresA15toA10 = (combined_lores_mask) & (HBL | (ds->page_bit >> 3));
    uint32_t HiresA15toA10 = (combined_hires_mask) & (ds->page_bit | (VCVBVA << 10));
    uint32_t A15toA10 = LoresA15toA10 | HiresA15toA10;

    uint32_t address = A2toA0 | A6toA3 | A9toA7 | A15toA10;

    ds->video_byte = cpu->mmu->read_raw(address);

    bool display_text = (ds->display_mode == TEXT_MODE) || (mixed_lores_mask != 0);

    if (display_text) {
        if (text_cycles >= (192*65*2)) // 2 frames
            ds->kill_color = true;
        else
            ++text_cycles;
    }
    else {
        text_cycles = 0;
        ds->kill_color = false;
    }

    ds->video_blanking = 0; // This is IIe. 0x80 for IIgs
    if (ds->vcount >= 0x100 && ds->vcount < 0x1C0)
    {
        ds->video_blanking = 0x80; // This is IIe. 0 for IIgs
        if (ds->hcount >= 0x58)
        {
            uint16_t vidbits = 0;

            if (display_text)
            {
                uint8_t vbyte = (*cpu->rd->char_rom_data)[8 * ds->video_byte + VCVBVA];

                // for inverse, xor the pixels with 0xFF to invert them.
                if ((ds->video_byte & 0xC0) == 0) {  // inverse
                    vbyte ^= 0xFF;
                } else if (((ds->video_byte & 0xC0) == 0x40)) {  // flash
                    vbyte ^= ds->flash_state ? 0xFF : 0;
                }
                for (int count = 7; count; --count) {
                    vidbits = (vidbits << 1) | (vbyte & 1);
                    vidbits = (vidbits << 1) | (vbyte & 1);
                    vbyte >>= 1;
                }
            }
            else if (LoresA15toA10)
            {
                uint8_t vbyte = ds->video_byte;
                vbyte = (vbyte >> (VCVBVA & 4)) & 0x0F; // hi or lo nibble
                vbyte = (vbyte << 1) | (ds->hcount & 1); // even/odd columns
                vidbits = lgr_bits[vbyte];
            }
            else {
                vidbits = hgr_bits[ds->video_byte];
            }

            //printf("col:%u, row:%u\n", ds->vcount - 0x100, ds->hcount - 0x58);
            ds->vidbits[ds->vcount - 0x100][ds->hcount - 0x58] = vidbits;
        }
    }
}

#if 0
void test_mega_ii_cycle(cpu_state *cpu)
{
    uint32_t address;
    int times = 17030;

    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);

    while (times) {
        --times;
        //address = mega_ii_cycle(cpu);
        /*
        if (ds->hcount == 0) {
            printf("vcount:%4.4x hcount:%4.4x at %4.4x\n", ds->vcount, ds->hcount, address);
            //if (vcount == 0x1FF) break;
        }
        */
    }
}
#endif
