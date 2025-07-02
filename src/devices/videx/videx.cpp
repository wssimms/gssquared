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

#include "gs2.hpp"
#include "cpu.hpp"
#include "videx.hpp"
#include "debug.hpp"
#include "display/DisplayBase.hpp"
#include "videx_80x24.hpp"
#include "videosystem.hpp"
#include "devices/annunciator/annunciator.hpp"
#include "devices/displaypp/RGBA.hpp"

void DisplayVidex::videx_set_line_dirty(int line) {
    if (line>=0 && line<24) {
        line_dirty[line] = true;
    }
}

void DisplayVidex::videx_set_line_dirty_by_addr(uint16_t addr) { /* addr is address in 2048 video memory */

    uint16_t start = reg[R12_START_ADDR_HI] << 8 | reg[R13_START_ADDR_LO];
    uint16_t offset = (addr - start) & 0x7FF;
    int line = offset / 80;

    // this can reference memory that is outside our 24 lines, in that case, ignore.
    if (line>=0 && line<24) {
        line_dirty[line] = true;
    }
}


/**
Videx Memory Mapping of C800-C8FF:
CB00 - CBFF, Cs00 - CsFF: bytes $300-$3FF of ROM file
C800 - CAFF: bytes $000-$2FF of ROM file
CC00 - CDFF: 512 byte page N of screen memory
CEFF - CFFF: unused
*/

void map_rom_videx(void *context, SlotType_t slot) {
    DisplayVidex * videx_d = (DisplayVidex *)context;

    uint8_t *dp = videx_d->rom->get_data();

    videx_d->mmu->map_c1cf_page_read_only(0xC8, dp + 0x000, "VIDEXROM");
    videx_d->mmu->map_c1cf_page_read_only(0xC9, dp + 0x100, "VIDEXROM");
    videx_d->mmu->map_c1cf_page_read_only(0xCA, dp + 0x200, "VIDEXROM");
    videx_d->mmu->map_c1cf_page_read_only(0xCB, dp + 0x300, "VIDEXROM");
    videx_d->mmu->map_c1cf_page_both(0xCC, nullptr, "NONE");
    videx_d->mmu->map_c1cf_page_both(0xCD, nullptr, "NONE");
    videx_d->mmu->map_c1cf_page_both(0xCE, nullptr, "NONE");
    videx_d->mmu->map_c1cf_page_both(0xCF, nullptr, "NONE");


    videx_d->mmu->map_c1cf_page_write_h(0xCC, { videx_memory_write2, videx_d }, "VIDEXRAM");
    videx_d->mmu->map_c1cf_page_write_h(0xCD, { videx_memory_write2, videx_d }, "VIDEXRAM");
    videx_d->mmu->map_c1cf_page_read_h(0xCC, { videx_memory_read2, videx_d }, "VIDEXRAM");
    videx_d->mmu->map_c1cf_page_read_h(0xCD, { videx_memory_read2, videx_d }, "VIDEXRAM");
    videx_d->mmu->dump_page_table(0xC8,0xCF);
    
    //if (DEBUG(DEBUG_VIDEX)) fprintf(stdout, "mapped in videx $C800-$CFFF\n");
}

void update_videx_screen_memory(DisplayVidex * videx_d) {
    videx_d->mmu->map_c1cf_page_read_only(0xCC, videx_d->screen_memory + (videx_d->selected_page * 2) * 0x100, "VIDEXRAM");
    videx_d->mmu->map_c1cf_page_read_only(0xCD, videx_d->screen_memory + (videx_d->selected_page * 2) * 0x100 + 0x100, "VIDEXRAM");
}

uint8_t videx_read_C0xx(void *context, uint16_t addr) {
    DisplayVidex * videx_d = (DisplayVidex *)context;
    uint8_t slot = (addr - 0xC080) >> 4;

    uint8_t reg = (addr & 0x0F);
    if (reg == VIDEX_REG_VAL) {
        return videx_d->reg[videx_d->selected_register];
    } else {
        uint8_t pg = reg >> 2;
        videx_d->selected_page = (videx_page_t) pg;
        update_videx_screen_memory(videx_d);
        if (DEBUG(DEBUG_VIDEX)) fprintf(stdout, "videx_read_C0xx: %04X %d\n", addr, pg);
    }
    
    return 0x00; // TODO: return floating bus - one emu says return 0? openemu returns floating for sure.
}

void videx_write_C0xx(void *context, uint16_t addr, uint8_t data) {
    DisplayVidex * videx_d = (DisplayVidex *)context;
    uint8_t slot = (addr - 0xC080) >> 4;
    uint8_t reg = addr & 0xF;

    if (DEBUG(DEBUG_VIDEX)) fprintf(stdout, "videx_write_C0xx: %04X %d\n", addr, data);
    if (reg == VIDEX_REG_ADDR) {
        videx_d->selected_register = data;
        if (DEBUG(DEBUG_VIDEX)) fprintf(stdout, "selected register: %d\n", videx_d->selected_register);
    } else if (reg == VIDEX_REG_VAL) {
        int selreg = videx_d->selected_register;    
        
        if ((selreg == R14_CURSOR_HI) || (selreg == R15_CURSOR_LO)) {
            /* whichever line the cursor was on */
            videx_d->videx_set_line_dirty_by_addr(videx_d->reg[R14_CURSOR_HI] << 8 | videx_d->reg[R15_CURSOR_LO]);
        }
 
        videx_d->reg[selreg] = data;
 
        if ((selreg == R10_CURSOR_START) || (selreg == R11_CURSOR_END)) {
             /* whichever line the cursor is now on */
            videx_d->videx_set_line_dirty_by_addr(videx_d->reg[R14_CURSOR_HI] << 8 | videx_d->reg[R15_CURSOR_LO]);
        }
        if ((selreg == R12_START_ADDR_HI) || (selreg == R13_START_ADDR_LO)) { /* all lines are dirty */
            for (int line = 0; line < 24; line++) {
                videx_d->videx_set_line_dirty(line);
            }
        }
        if ((selreg == R14_CURSOR_HI) || (selreg == R15_CURSOR_LO)) {
            /* whichever line the cursor was on */
            videx_d->videx_set_line_dirty_by_addr(videx_d->reg[R14_CURSOR_HI] << 8 | videx_d->reg[R15_CURSOR_LO]);
        }
        if (DEBUG(DEBUG_VIDEX)) fprintf(stdout, "register %02X set to %02x\n", videx_d->selected_register, data);
    }
}

static RGBA_t p_white = {.a=0xFF, .b=0xFF, .g=0xFF, .r=0xFF};

void DisplayVidex::render_videx_scanline_80x24(cpu_state *cpu, int y, void *pixels, int pitch) {
    //videx_data * videx_d = (videx_data *)get_module_state(cpu, MODULE_VIDEX);
    RGBA_t *texturePixels = (RGBA_t *)pixels;

    RGBA_t color_value = p_white;

    /**
     * calculate memory address of start of line:
     * y * 80 + reg[VIDEX_REG_START_ADDR_LO] | reg[VIDEX_REG_START_ADDR_HI] >> 8
     * modulus 2048
     */

    uint16_t start_addr_lo = reg[R13_START_ADDR_LO];
    uint16_t start_addr_hi = reg[R12_START_ADDR_HI];
    uint16_t line_start = (y * 80 + ((uint16_t)start_addr_hi << 8 | start_addr_lo)) % 2048;

    /**
     * if R10[6] is set, cursor blink is enabled
     *    R10[5] = 0: blink at 1/16th rate
     *    R10[5] = 1: blink at 1/32nd rate
     * if R10[6] is clear, no blinking
     *    R10[5] = 0: cursor is displayed
     *    R10[5] = 1: cursor is not displayed
     */
    uint8_t cursor_start = reg[R10_CURSOR_START] & 0b00011111;
    uint8_t cursor_end = reg[R11_CURSOR_END] & 0b00011111;

    uint16_t cursor_addr_lo = reg[R15_CURSOR_LO];
    uint16_t cursor_addr_hi = reg[R14_CURSOR_HI];
    uint16_t cursor_pos = ((uint16_t)cursor_addr_hi << 8 | cursor_addr_lo) % 2048;
    bool cursor_blink_mode = (reg[R10_CURSOR_START] & 0b01000000) != 0;
    bool cursor_enabled =    (reg[R10_CURSOR_START] & 0b00100000) != 0;
    bool cursor_visible = (!cursor_blink_mode && !cursor_enabled) || (cursor_blink_mode && cursor_blink_status);   

    //printf("line_start for HI: %02X LO: %02X y: %d: %d\n", start_addr_hi, start_addr_lo, y, line_start);
    for (int ln = 0; ln < 9; ln++) {
        for (int x = 0; x < 80; x++) {
            uint16_t char_addr = (line_start + x) % 2048;
            uint8_t character = screen_memory[char_addr];
            uint8_t ch = character & 0x7F;

            uint8_t cmap = (character & 0x80)>0 ? alt_char_set[((ch * 16) + ln)] : char_set[((ch * 16) + ln)];

            bool is_cursor = (cursor_pos == char_addr);
            bool cursor_this_char = (cursor_visible && is_cursor && (ln >= cursor_start && ln <= cursor_end));

            for (int px = 0; px < 8; px++) {
                if (cursor_this_char) {
                    *texturePixels = (cmap & 0x80) ? RGBA_t{0,0,0,0} : color_value;
                } else {
                    *texturePixels = (cmap & 0x80) ? color_value : RGBA_t{0,0,0,0};
                }
                cmap <<= 1;
                texturePixels++;
            }

        }
    }
}

void DisplayVidex::videx_render_line(cpu_state *cpu, int y)
{
    // the texture is our canvas. When we 'lock' it, we get a pointer to the pixels, and the pitch which is pixels per row
    // of the area. Since all our chars are the same we can just use the same pitch for all our chars.
    /* void* pixels = buffer + (y * 9 * VIDEX_SCREEN_WIDTH * 4); */
    void* pixels = (buffer + (y * 9 * VIDEX_SCREEN_WIDTH));
    render_videx_scanline_80x24(cpu, y, pixels, VIDEX_SCREEN_WIDTH * 4);
}

/**
 * Update Display: for Display System Videx.
 * Called once per frame (16.67ms, 60fps) to update the display.
 */
void DisplayVidex::update_display_videx(cpu_state *cpu) {
    video_system_t *vs = video_system;

// openemulator disagrees, claims bit 5 = 1 means display cursor. But the manual clearly says bit 5 = 0 means cursor is on.
    bool cursor_blink_mode = (reg[R10_CURSOR_START] & 0b01000000) != 0;
    bool cursor_enabled =    (reg[R10_CURSOR_START] & 0b00100000) != 0;

    if (cursor_blink_mode) { // start of field
        cursor_blink_count++;
        bool cursor_updated = false;
        if (cursor_enabled && cursor_blink_count >= 16) { // TODO: is this a blink on and blink off each 1/32? in which case, my counter should be half.
            cursor_blink_status = !cursor_blink_status;
            cursor_blink_count = 0;
            cursor_updated = true;
        } else if (!cursor_enabled && cursor_blink_count >= 8) {
            cursor_blink_status = !cursor_blink_status;
            cursor_blink_count = 0;
            cursor_updated = true;
        }
        if (cursor_updated) {
            videx_set_line_dirty_by_addr(reg[R14_CURSOR_HI] << 8 | reg[R15_CURSOR_LO]);
        }
    }

    int framedirty = 0;
    for (int line = 0; line < 24; line++) {
        if (vs->force_full_frame_redraw || line_dirty[line]) {
            videx_render_line(cpu, line);
            line_dirty[line] = false;
            framedirty=1;
        }
    }
// copy buffer into texture in one go.
    if (framedirty) {
        void* pixels;
        int pitch;
        if (!SDL_LockTexture(screenTexture, NULL, &pixels, &pitch)) {
            fprintf(stderr, "Failed to lock texture: %s\n", SDL_GetError());
            return;
        }
        memcpy(pixels, buffer, VIDEX_SCREEN_WIDTH * VIDEX_SCREEN_HEIGHT * sizeof(RGBA_t));
        SDL_UnlockTexture(screenTexture);
    }
    vs->force_full_frame_redraw = false;

    SDL_SetTextureBlendMode(screenTexture, SDL_BLENDMODE_ADD); // double-draw this to increase brightness.
    vs->render_frame(screenTexture, 0.0f);
    vs->render_frame(screenTexture, 0.0f);
}

bool DisplayVidex::update_display(cpu_state *cpu)
{
    VideoScannerII *video_scanner = cpu->get_video_scanner();
    
    annunciator_state_t * anc_d = (annunciator_state_t *)get_module_state(cpu, MODULE_ANNUNCIATOR);

    // the backbuffer must be cleared each frame. The docs state this clearly
    // but I didn't know what the backbuffer was. Also, I assumed doing it once
    // at startup was enough. NOPE.
    video_system->clear();

    //if (videx_d && ds->display_mode == TEXT_MODE && anc_d && anc_d->annunciators[0] ) {
    if (video_scanner->is_text() && anc_d && anc_d->annunciators[0] ) {
        update_display_videx(cpu); 
        return true;
    }
    return false;
}

DisplayVidex::DisplayVidex(computer_t *computer) :
    Display(computer, VIDEX_SCREEN_WIDTH, VIDEX_SCREEN_HEIGHT)
{    
    static uint8_t registers_init[18] = {
         0x7B,        0x50,        0x62,        0x29,        0x1B,
         0x08,        0x18,        0x19,        0x00,        0x08,
        0xC0,       0x08,       0x00,       0x00,       0x00,
        0x00,       0x00,       0x00,
    };

    printf("Videx video_system is: %p\n", video_system);
    fflush(stdout);

    mmu = computer->mmu;
    cpu = computer->cpu;

    id = DEVICE_ID_DISPLAY_VIDEX;

    screen_memory = new uint8_t[VIDEX_SCREEN_MEMORY];
    char_memory = new uint8_t[VIDEX_CHAR_SET_COUNT * VIDEX_CHARSET_SIZE];

    selected_register = 0;
    selected_page = VIDEX_PAGE0;

    for (int i = 0; i < 18; i++)
        reg[i] = registers_init[i];

    rom = new ResourceFile("roms/cards/videx/videx-2.4.rom", READ_ONLY);
    if (rom == nullptr) {
        fprintf(stderr, "Failed to load videx-2.4.rom\n");
        return;
    }
    rom->load();

    // load all the character set ROMs we have into memory. Provide a routine to select which one to use.
    for (int i = 0; i < VIDEX_CHAR_SET_COUNT; i++) {
        char_sets[i] = new ResourceFile(videx_char_roms[i].filename, READ_ONLY);
        if (char_sets[i] == nullptr) {
            fprintf(stderr, "Failed to load %s\n", videx_char_roms[i].filename);
            return;
        }
        char_sets[i]->load();
        memcpy(char_memory + (i * VIDEX_CHARSET_SIZE), char_sets[i]->get_data(), VIDEX_CHARSET_SIZE);
    }

    char_set = char_memory;
    alt_char_set = char_memory + 2048;
}

DisplayVidex::~DisplayVidex()
{
    for (int i = 0; i < VIDEX_CHAR_SET_COUNT; i++) {
        if (char_sets[i] != nullptr) {
            delete char_sets[i];
        }
    }
    delete screen_memory;
    delete char_memory;
    delete rom;
}

void init_slot_videx(computer_t *computer, SlotType_t slot)
{
    DisplayVidex * videx = new DisplayVidex(computer);
    printf("Videx is: %p\n", videx);
    printf("Videx width: %d  height: %d\n", videx->get_width(), videx->get_height());
    fflush(stdout);

    set_slot_state(videx->cpu, slot, videx);
    fprintf(stdout, "init_slot_videx %d\n", slot);
   
    // memory-map the page. Refactor to have a method to get and set memory map.
    uint8_t *rom_data = videx->rom->get_data();

    // load the firmware into the slot memory -- refactor this
    computer->mmu->set_slot_rom(slot, rom_data+0x0300, "VIDEXROM");

    uint16_t slot_base = 0xC080 + (slot * 0x10);

    computer->mmu->set_C0XX_read_handler(slot_base + VIDEX_PAGE_SELECT_0, { videx_read_C0xx, videx });
    computer->mmu->set_C0XX_read_handler(slot_base + VIDEX_PAGE_SELECT_1, { videx_read_C0xx, videx });
    computer->mmu->set_C0XX_read_handler(slot_base + VIDEX_PAGE_SELECT_2, { videx_read_C0xx, videx });
    computer->mmu->set_C0XX_read_handler(slot_base + VIDEX_PAGE_SELECT_3, { videx_read_C0xx, videx });
    computer->mmu->set_C0XX_read_handler(slot_base + VIDEX_REG_VAL, { videx_read_C0xx, videx });

    // register the write handler for C0xx
    computer->mmu->set_C0XX_write_handler(slot_base + VIDEX_REG_ADDR, { videx_write_C0xx, videx });
    computer->mmu->set_C0XX_write_handler(slot_base + VIDEX_REG_VAL, { videx_write_C0xx, videx });

    computer->mmu->set_C8xx_handler(slot, map_rom_videx, videx );

    computer->register_shutdown_handler([videx]() {
        delete videx;
        return true;
    });

    videx->register_display_device(computer, DEVICE_ID_DISPLAY_VIDEX);
}
