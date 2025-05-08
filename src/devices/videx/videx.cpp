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
#include "bus.hpp"
#include "videx.hpp"
#include "memory.hpp"
#include "debug.hpp"
#include "display/display.hpp"


void videx_set_line_dirty(videx_data * videx_d, int line) {
    if (line>=0 && line<24) {
        videx_d->line_dirty[line] = true;
    }
}

void videx_set_line_dirty_by_addr(videx_data * videx_d, uint16_t addr) { /* addr is address in 2048 video memory */
    //videx_data * videx_d = (videx_data *)get_module_state(cpu, MODULE_VIDEX);
    uint16_t start = videx_d->reg[R12_START_ADDR_HI] << 8 | videx_d->reg[R13_START_ADDR_LO];
    uint16_t offset = (addr - start) & 0x7FF;
    int line = offset / 80;
    //printf("videx_set_line_dirty_by_addr: %04X => %d dirty\n", addr, line);
    // this can reference memory that is outside our 24 lines, in that case, ignore.
    if (line>=0 && line<24) {
        videx_d->line_dirty[line] = true;
    }
}

void map_rom_videx(cpu_state *cpu, SlotType_t slot) {
    // CB00 - CBFF, Cs00 - CsFF: bytes $300-$3FF of ROM file
    // C800 - CAFF: bytes $000-$2FF of ROM file
    // CC00 - CDFF: 512 byte page N of screen memory
    // CEFF - CFFF: unused

    videx_data * videx_d = (videx_data *)get_slot_state(cpu, slot);
    uint8_t *dp = videx_d->rom->get_data();
    memory_map_page_both(cpu, 0xC8, dp + 0x000, MEM_IO);
    memory_map_page_both(cpu, 0xC9, dp + 0x100, MEM_IO);
    memory_map_page_both(cpu, 0xCA, dp + 0x200, MEM_IO);
    memory_map_page_both(cpu, 0xCB, dp + 0x300, MEM_IO);
    
    memory_map_page_both(cpu, 0xCC, videx_d->screen_memory + (videx_d->selected_page * 2) * 0x100, MEM_IO);
    memory_map_page_both(cpu, 0xCD, videx_d->screen_memory + (videx_d->selected_page * 2) * 0x100 + 0x100, MEM_IO);
    
    //if (DEBUG(DEBUG_VIDEX)) fprintf(stdout, "mapped in videx $C800-$CFFF\n");
}

void update_videx_screen_memory(cpu_state *cpu, videx_data * videx_d) {
    memory_map_page_both(cpu, 0xCC, videx_d->screen_memory + (videx_d->selected_page * 2) * 0x100, MEM_IO);
    memory_map_page_both(cpu, 0xCD, videx_d->screen_memory + (videx_d->selected_page * 2) * 0x100 + 0x100, MEM_IO);
}

uint8_t videx_read_C0xx(cpu_state *cpu, uint16_t addr) {
    uint8_t slot = (addr - 0xC080) >> 4;
    videx_data * videx_d = (videx_data *)get_slot_state(cpu, (SlotType_t)slot);
    
    uint8_t reg = (addr & 0x0F);
    if (reg == VIDEX_REG_VAL) {
        return videx_d->reg[videx_d->selected_register];
    } else {
        uint8_t pg = reg >> 2;
        videx_d->selected_page = (videx_page_t) pg;
        update_videx_screen_memory(cpu, videx_d);
        if (DEBUG(DEBUG_VIDEX)) fprintf(stdout, "videx_read_C0xx: %04X %d\n", addr, pg);
    }
    
    return 0x00; // TODO: return floating bus - one emu says return 0? openemu returns floating for sure.
}

void videx_write_C0xx(cpu_state *cpu, uint16_t addr, uint8_t data) {
    uint8_t slot = (addr - 0xC080) >> 4;
    videx_data * videx_d = (videx_data *)get_slot_state(cpu, (SlotType_t)slot);
    uint8_t reg = addr & 0xF;

    if (DEBUG(DEBUG_VIDEX)) fprintf(stdout, "videx_write_C0xx: %04X %d\n", addr, data);
    if (reg == VIDEX_REG_ADDR) {
        videx_d->selected_register = data;
        if (DEBUG(DEBUG_VIDEX)) fprintf(stdout, "selected register: %d\n", videx_d->selected_register);
    } else if (reg == VIDEX_REG_VAL) {
        int selreg = videx_d->selected_register;    
        
        if ((selreg == R14_CURSOR_HI) || (selreg == R15_CURSOR_LO)) {
            /* whichever line the cursor was on */
            videx_set_line_dirty_by_addr(videx_d, videx_d->reg[R14_CURSOR_HI] << 8 | videx_d->reg[R15_CURSOR_LO]);
        }
 
        videx_d->reg[selreg] = data;
 
        if ((selreg == R10_CURSOR_START) || (selreg == R11_CURSOR_END)) {
             /* whichever line the cursor is now on */
            videx_set_line_dirty_by_addr(videx_d, videx_d->reg[R14_CURSOR_HI] << 8 | videx_d->reg[R15_CURSOR_LO]);
        }
        if ((selreg == R12_START_ADDR_HI) || (selreg == R13_START_ADDR_LO)) { /* all lines are dirty */
            for (int line = 0; line < 24; line++) {
                videx_set_line_dirty(videx_d, line);
            }
        }
        if ((selreg == R14_CURSOR_HI) || (selreg == R15_CURSOR_LO)) {
            /* whichever line the cursor was on */
            videx_set_line_dirty_by_addr(videx_d, videx_d->reg[R14_CURSOR_HI] << 8 | videx_d->reg[R15_CURSOR_LO]);
        }
        if (DEBUG(DEBUG_VIDEX)) fprintf(stdout, "register %02X set to %02x\n", videx_d->selected_register, data);
    }
}

void init_slot_videx(cpu_state *cpu, SlotType_t slot) {
    videx_data * videx_d = new videx_data;
    video_system_t *vs = cpu->video_system;

    videx_d->id = DEVICE_ID_VIDEX;
    // set in CPU so we can reference later
    videx_d->screen_memory = new uint8_t[VIDEX_SCREEN_MEMORY];
    videx_d->selected_register = 0;
    videx_d->selected_page = VIDEX_PAGE0;

    videx_d->char_memory = new uint8_t[VIDEX_CHAR_SET_COUNT * VIDEX_CHARSET_SIZE];

    uint8_t registers_init[18] = {
        0x7B,         0x50,        0x62,        0x29,        0x1B,
        0x08,        0x18,        0x19,        0x00,        0x08,
        0xC0,        0x08,        0x00,        0x00,        0x00,
        0x00,        0x00,        0x00,
    };

    for (int i = 0; i < 18; i++) videx_d->reg[i] = registers_init[i];

    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);

    // Create the screen texture for Videx
    videx_d->videx_texture = SDL_CreateTexture(vs->renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_STREAMING,
        VIDEX_SCREEN_WIDTH, VIDEX_SCREEN_HEIGHT);

    if (!videx_d->videx_texture) {
        fprintf(stderr, "Error creating screen texture: %s\n", SDL_GetError());
        return;
    }
    SDL_SetTextureScaleMode(videx_d->videx_texture, SDL_SCALEMODE_LINEAR);

    ResourceFile *rom = new ResourceFile("roms/cards/videx/videx-2.4.rom", READ_ONLY);
    if (rom == nullptr) {
        fprintf(stderr, "Failed to load videx-2.4.rom\n");
        return;
    }
    rom->load();
    videx_d->rom = rom;

    // load all the character set ROMs we have into memory. Provide a routine to select which one to use.
    for (int i = 0; i < VIDEX_CHAR_SET_COUNT; i++) {
        videx_d->char_sets[i] = new ResourceFile(videx_char_roms[i].filename, READ_ONLY);
        if (videx_d->char_sets[i] == nullptr) {
            fprintf(stderr, "Failed to load %s\n", videx_char_roms[i].filename);
            return;
        }
        videx_d->char_sets[i]->load();
        memcpy(videx_d->char_memory + (i * VIDEX_CHARSET_SIZE), videx_d->char_sets[i]->get_data(), VIDEX_CHARSET_SIZE);
    }

    videx_d->char_set = videx_d->char_memory;
    videx_d->alt_char_set = videx_d->char_memory + 2048;

    set_slot_state(cpu, slot, videx_d);

    fprintf(stdout, "init_slot_videx %d\n", slot);

    // Todo: we need to get triggered whenever any videx RAM is changed, to do a video
    // scanline update. How?
    
    // memory-map the page. Refactor to have a method to get and set memory map.
    uint8_t *rom_data = videx_d->rom->get_data();
    /* memory_map_page_both(cpu, slot + 0xC0, rom_data + (slot*GS2_PAGE_SIZE), MEM_ROM); */

    // load the firmware into the slot memory -- refactor this
    for (int i = 0; i < 256; i++) {
        raw_memory_write(cpu, 0xC000 + (slot * 0x0100) + i, rom_data[i + 0x0300]);
    }

    uint16_t slot_base = 0xC080 + (slot * 0x10);

    // register the read handler for C0xx
    register_C0xx_memory_read_handler(slot_base + VIDEX_PAGE_SELECT_0, videx_read_C0xx);
    register_C0xx_memory_read_handler(slot_base + VIDEX_PAGE_SELECT_1, videx_read_C0xx);
    register_C0xx_memory_read_handler(slot_base + VIDEX_PAGE_SELECT_2, videx_read_C0xx);
    register_C0xx_memory_read_handler(slot_base + VIDEX_PAGE_SELECT_3, videx_read_C0xx);
    register_C0xx_memory_read_handler(slot_base + VIDEX_REG_VAL, videx_read_C0xx);

    // register the write handler for C0xx
    register_C0xx_memory_write_handler(slot_base + VIDEX_REG_ADDR, videx_write_C0xx);
    register_C0xx_memory_write_handler(slot_base + VIDEX_REG_VAL, videx_write_C0xx);

    register_C8xx_handler(cpu, slot, map_rom_videx);
}
