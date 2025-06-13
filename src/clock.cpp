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

int made_bits = 0;
int text_cycles = 0;

//uint32_t hcount = 0x7F;  //     HPE' H5 H4 H3 H2 H1 H0
//uint32_t vcount = 0xFF;  // V5 V4 V3 V2 V1 V0 VC VB VA

void counter(cpu_state *cpu)
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

    // uint32_t VBL    = ((vcount >> 3) & 1) & ((vcount >> 4) & 1);

    // A2-A0 = H2-H0
    uint32_t A2A1A0 = ds->hcount & 7;

    //     1  1  0  1
    //       H5 H4 H3
    // +  V4 V3 V4 V3
    // --------------
    //    A6 A5 A4 A3
    uint32_t V3V4V3V4 = ((ds->vcount & 0xC0) >> 1) | ((ds->vcount & 0xC0) >> 3);
    uint32_t A6A5A4A3 = (0x68 + (ds->hcount & 0x38) + V3V4V3V4) & 0x78;

    // A9-A7 = V2-V0
    uint32_t A9A8A7 = (ds->vcount & 0x38) << 4;

    // HBL = (H4' | H3') & H5'
    uint32_t NOTH   = (ds->hcount ^ 0x7F);
    uint32_t HBL    = (((NOTH << 9) | (NOTH << 8)) & (NOTH << 7)) & 0x1000;

    // mixlines = V2 & V4 [set in lines 160-291 and 224-261]
    uint32_t mixlines = ((ds->vcount & 0x20) << 5) & ((ds->vcount & 0x80) << 3);
    // mixtxt = 0x1C00 in lines 160-291 and 224-261 else mixtxt = 0x0000
    uint32_t mixtxt   = mixlines | (mixlines << 1) | (mixlines << 2);
    // mixhgr = 0x0000 in lines 160-291 and 224-261 else mixhgr = 0x7C00
    uint32_t mixhgr   = ((mixtxt | (mixtxt << 2)) ^ 0x07C00);

    /* This is what I want:
    if (ds->full_bits) {
        LoresA10toA15 = ds->lores_bits & ds->page_bits & HBL;
        HiresA10toA15 = ds->hires_bits & ds->page_bits & (vcount << 4);
    }
    else {
        LoresA10toA15 = (mixtxt | ds->lores_bits) & ds->page_bits & HBL;
        HiresA10toA15 = (mixhgr & ds->hires_bits) & ds->page_bits & (vcount << 4);
    }
    that is, in mixed mode, get text/lores addresses if either we're not in hires
    or we are, but in the mixed area. and get hires address if we are both in hires
    mode and not in the mixed area.
    */

    // if full_bits then
    //     force mixtxt to all zeros so that (mixtxt | ds->lores_bits) == ds->lores_bits
    //     force mixhgr to all ones so that (mixhgr & ds->hires_bits) == ds->hires_bits
    mixtxt = mixtxt & (~ds->full_bits);
    mixhgr = mixhgr | ds->full_bits;

    uint32_t VCVBVA = ds->vcount & 7;
    uint32_t LoresA15toA10 = (mixtxt | ds->lores_bits) & ((ds->page_bits >> 3) | HBL);
    uint32_t HiresA15toA10 = (mixhgr & ds->hires_bits) & (ds->page_bits | (VCVBVA << 10));

    uint32_t address = A2A1A0 | A6A5A4A3 | A9A8A7 | LoresA15toA10 | HiresA15toA10;

    ds->video_byte = cpu->mmu->read_raw(address);

    bool display_text = ((ds->display_mode == TEXT_MODE) || (mixtxt != 0));

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

void test_counter(cpu_state *cpu)
{
    uint32_t address;
    int times = 17030;

    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);

    while (times) {
        --times;
        //address = counter(cpu);
        /*
        if (ds->hcount == 0) {
            printf("vcount:%4.4x hcount:%4.4x at %4.4x\n", ds->vcount, ds->hcount, address);
            //if (vcount == 0x1FF) break;
        }
        */
    }
}

void test_mega_ii_cycle(cpu_state *cpu)
{
    int times = 17030;

    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);

    while (times) {
        --times;
        mega_ii_cycle(cpu);
    }
}

void mega_ii_cycle(cpu_state *cpu)
{
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);

    if (!made_bits) {
        // test_counter(cpu);
        // lazy initialization
        make_flipped();
        make_hgr_bits();
        make_lgr_bits();
        ++made_bits;
    }

    cpu->cycles++;

    ds->horz_counter++;
    if (ds->horz_counter == 65) {
        ds->horz_counter = 0;
        ds->vert_counter++;
        if (ds->vert_counter == 262) {
            ds->vert_counter = 0;
        }
    }

    int display_text = (ds->display_mode == TEXT_MODE);
    if (ds->display_split_mode == SPLIT_SCREEN) {
        display_text |= (ds->vert_counter < 34);
        display_text |= (ds->vert_counter >= 194 && ds->vert_counter < 226);
        display_text |= (ds->vert_counter >= 258);
    }

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

    uint32_t line_addr = 0;

    if (display_text || ds->display_graphics_mode == LORES_MODE) {
        line_addr = txt_vcount_addr[ds->vert_counter];
        if (ds->display_page_num == DISPLAY_PAGE_2) line_addr += 0x400;
    }
    else {
        line_addr = hgr_vcount_addr[ds->vert_counter];
        if (ds->display_page_num == DISPLAY_PAGE_2) line_addr += 0x2000;
    }

    if (ds->horz_counter < 25) {
        uint32_t base_addr = line_addr;
        line_addr -= 0x18;
        if ((base_addr >> 7) != (line_addr >> 7))
            line_addr += 0x80;
        if (display_text || ds->display_graphics_mode == LORES_MODE)
            line_addr += 0x1000;
    }

    line_addr += horz_count_offs[ds->horz_counter];
    ds->video_byte = cpu->mmu->read_raw(line_addr);

    int row = ds->vert_counter - 34;
    int col = ds->horz_counter - 25;
    int minor = (ds->vert_counter + 30) % 8;

    ds->video_blanking = 0; // This is IIe. 0x80 for IIgs
    if (ds->vert_counter >= 34 && ds->vert_counter < 226)
    {
        ds->video_blanking = 0x80; // This is IIe. 0 for IIgs
        if (ds->horz_counter >= 25)
        {
            uint16_t vidbits = 0;

            if (display_text)
            {
                uint8_t vbyte = (*cpu->rd->char_rom_data)[8 * ds->video_byte + minor];

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
            else if (ds->display_graphics_mode == LORES_MODE)
            {
                uint8_t vbyte = ds->video_byte;
                vbyte = (vbyte >> (minor & 4)) & 0x0F; // hi or lo nibble
                vbyte = (vbyte << 1) | (col & 1); // even/odd columns
                vidbits = lgr_bits[vbyte];
            }
            else // GRAPHICS_MODE && HIRES_MODE
            {
                vidbits = hgr_bits[ds->video_byte];
            }

            ds->vidbits[row][col] = vidbits;
        }
    }
}