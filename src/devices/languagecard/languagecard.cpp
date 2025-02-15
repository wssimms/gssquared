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

#include <stdio.h>
#include "cpu.hpp"
#include "bus.hpp"
#include "memory.hpp"
#include "debug.hpp"

#include "devices/languagecard/languagecard.hpp"


/* uint8_t bank_selected = 0; */
/* uint8_t reading_ram_rom = 0; */


void debug_memory_pointer(cpu_state *cpu, uint8_t *pointer) {
    if (pointer >= cpu->main_ram_64 && pointer < (cpu->main_ram_64 + 0xC000)) {
        uint16_t page = (pointer - cpu->main_ram_64) / GS2_PAGE_SIZE;
        printf("main_ram_64 [page]: %02X ", page);
        return;
    }
    if (pointer >= cpu->main_ram_64 + 0xC000 && pointer < (cpu->main_ram_64 + 0xD000)) {
        uint16_t page = (pointer - cpu->main_ram_64) / GS2_PAGE_SIZE;
        printf("lc bank 1 [page]: %02X ", page + 0x10);
        return;
    }
    if (pointer >= cpu->main_ram_64 + 0xD000 && pointer < (cpu->main_ram_64 + 0xE000)) {
        uint16_t page = (pointer - cpu->main_ram_64 ) / GS2_PAGE_SIZE;
        printf("lc bank 2 [page]: %02X ", page);
        return;
    }
    if (pointer >= cpu->main_ram_64 + 0xE000 && pointer < (cpu->main_ram_64 + 0xFFFF)) {
        uint16_t page = (pointer - cpu->main_ram_64) / GS2_PAGE_SIZE;
        printf("lc bank E0-FF [page]: %02X ", page);
        return;
    }

    if (pointer >= cpu->main_rom_D0 && pointer < (cpu->main_rom_D0 + 0x3000)) {
        uint16_t page = (pointer - cpu->main_rom_D0) / GS2_PAGE_SIZE;
        printf("main_rom_D0 [page]: %02X ", page + 0xD0);
        return;
    }
    printf("unknown pointer: %p ", pointer);
}

/* static uint8_t FF_BANK_1 = 0;
static uint8_t FF_PRE_WRITE = 0;
static uint8_t FF_READ_ENABLE = 0;
static uint8_t _FF_WRITE_ENABLE = 0; // inverted sense */

void set_memory_pages_based_on_flags(cpu_state *cpu) {
    languagecard_state_t *lc = (languagecard_state_t *)get_module_state(cpu, MODULE_LANGCARD);

    uint8_t *bank = (lc->FF_BANK_1 == 1) ? cpu->main_ram_64 + 0xC000 : cpu->main_ram_64 + 0xD000;
    for (int i = 0; i < 16; i++) {
        if (lc->FF_READ_ENABLE) {
            // set pages_read[i] to bank[i*0x0100]
            cpu->memory->pages_read[i + 0xD0] = bank + (i*GS2_PAGE_SIZE);
            cpu->memory->page_info[i + 0xD0].type = MEM_RAM;
        } else { // reads == READ_ROM
            // set pages_read[i] to bank[i*0x0100]
            cpu->memory->pages_read[i + 0xD0] = cpu->main_rom_D0 + (i*GS2_PAGE_SIZE);
            cpu->memory->page_info[i + 0xD0].type = MEM_ROM;
        }

        if (!lc->_FF_WRITE_ENABLE) {
            cpu->memory->pages_write[i + 0xD0] = bank + (i*GS2_PAGE_SIZE);
            cpu->memory->page_info[i + 0xD0].type = MEM_RAM;
            cpu->memory->page_info[i + 0xD0].can_write = 1;
        } else { // writes == WRITE_NONE - set it to the ROM and can_write = 0
            cpu->memory->pages_write[i + 0xD0] = cpu->main_rom_D0 + (i*GS2_PAGE_SIZE);
            cpu->memory->page_info[i + 0xD0].type = MEM_ROM;
            cpu->memory->page_info[i + 0xD0].can_write = 0;
        }
    }

    for (int i = 0; i < 32; i++) {
        if (lc->FF_READ_ENABLE) {
            // set pages_read[i] to bank[i*0x0100]
            cpu->memory->pages_read[i + 0xE0] = cpu->main_ram_64 + ((i+0xE0)*GS2_PAGE_SIZE);
            cpu->memory->page_info[i + 0xE0].type = MEM_RAM;
        } else { // reads == READ_ROM
            // set pages_read[i] to bank[i*0x0100]
            cpu->memory->pages_read[i + 0xE0] = cpu->main_rom_D0 + ((i+0x10)*GS2_PAGE_SIZE);
            cpu->memory->page_info[i + 0xE0].type = MEM_ROM;
        }

        if (!lc->_FF_WRITE_ENABLE) {
            cpu->memory->pages_write[i + 0xE0] = cpu->main_ram_64 + ((i+0xE0)*GS2_PAGE_SIZE);
            cpu->memory->page_info[i + 0xE0].type = MEM_RAM;
            cpu->memory->page_info[i + 0xE0].can_write = 1;
        } else { // writes == WRITE_NONE - set it to the ROM and can_write = 0
            cpu->memory->pages_write[i + 0xE0] = cpu->main_rom_D0 + ((i+0x10)*GS2_PAGE_SIZE);
            cpu->memory->page_info[i + 0xE0].type = MEM_ROM;
            cpu->memory->page_info[i + 0xE0].can_write = 0;
        }
    }

    if (DEBUG(DEBUG_LANGCARD)) {
        for (int i = 0; i < 48; i+=16) {
            printf("page: %02X read: %p write: %p canwrite: %d ", 0xD0 + i,
                cpu->memory->pages_read[i + 0xD0], 
                cpu->memory->pages_write[i + 0xD0],
                cpu->memory->page_info[i + 0xD0].can_write
            );
            printf(" read "); debug_memory_pointer(cpu, cpu->memory->pages_read[i + 0xD0]);
            printf(" write "); debug_memory_pointer(cpu, cpu->memory->pages_write[i + 0xD0]);
            printf("\n");
        }
    }
}

uint8_t languagecard_read_C0xx(cpu_state *cpu, uint16_t address) {
    languagecard_state_t *lc = (languagecard_state_t *)get_module_state(cpu, MODULE_LANGCARD);

    if (DEBUG(DEBUG_LANGCARD)) printf("languagecard_read_%04X [%llu] ", address, cpu->cycles);

    if (address & LANG_A3 ) {
        // 1 = any access sets Bank_1
        lc->FF_BANK_1 = 1;
    } else {
        // 0 = any access resets Bank_1
        lc->FF_BANK_1 = 0;
    }

    if (((address & LANG_A0A1) == 0) || ((address & LANG_A0A1) == 3)) {
        // 00, 11 - set READ_ENABLE
        lc->FF_READ_ENABLE = 1;
    } else {
        // 01, 10 - reset READ_ENABLE
        lc->FF_READ_ENABLE = 0;
    }

    if (lc->FF_PRE_WRITE == 1 && (address & 0b00000001) == 1) {
        // 00000000 - reset WRITE_ENABLE'
        lc->_FF_WRITE_ENABLE = 0;
    } else {
        // 00000001 - set WRITE_ENABLE'
        lc->_FF_WRITE_ENABLE = 1;
    }

    if ((address & 0b00000001) == 1) {  // odd read access
        // 00000001 - set PRE_WRITE
        lc->FF_PRE_WRITE = 1;
    } else {                            // even access or write access
        // 00000000 - reset PRE_WRITE
        lc->FF_PRE_WRITE = 0;
    }

    if (DEBUG(DEBUG_LANGCARD)) printf("FF_BANK_1: %d, FF_READ_ENABLE: %d, FF_PRE_WRITE: %d, _FF_WRITE_ENABLE: %d\n", 
        lc->FF_BANK_1, lc->FF_READ_ENABLE, lc->FF_PRE_WRITE, lc->_FF_WRITE_ENABLE);
   
    /**
     * compose the memory map.
     * in main_ram:
     * 0x0000 - 0xBFFF - straight map.
     * 0xC000 - 0xCFFF - bank 1 extra memory
     * 0xD000 - 0xDFFF - bank 2 extra memory
     * 0xE000 - 0xFFFF - rest of extra memory
     * */

    set_memory_pages_based_on_flags(cpu);
    return 0;
}


void languagecard_write_C0xx(cpu_state *cpu, uint16_t address, uint8_t value) {
    languagecard_state_t *lc = (languagecard_state_t *)get_module_state(cpu, MODULE_LANGCARD);

    if (DEBUG(DEBUG_LANGCARD)) printf("languagecard_write_C0xx %04X value: %02X\n", address, value);
    lc->FF_PRE_WRITE = 0;

    if (address & LANG_A3 ) {
        // 1 = any access sets Bank_1 - read or write.
        lc->FF_BANK_1 = 1;
    } else {
        // 0 = any access resets Bank_1
        lc->FF_BANK_1 = 0;
    }

    if (((address & LANG_A0A1) == 0) || ((address & LANG_A0A1) == 3)) {
        // 00, 11 - set READ_ENABLE
        lc->FF_READ_ENABLE = 1;
    } else {
        // 01, 10 - reset READ_ENABLE
        lc->FF_READ_ENABLE = 0;
    }
    
    /* This means there is a feature of RAM card control
    not documented by Apple: write access to odd address in C08X
    range controls the READ ENABLE flip-flip without affecting the WRITE enable' flip-flop.
    STA $C081: no effect on write enable, disable read, bank 2 */
    if (address & 0b00000001) {
        lc->FF_READ_ENABLE = 0;
    }
    if (DEBUG(DEBUG_LANGCARD)) printf("FF_BANK_1: %d, FF_READ_ENABLE: %d, FF_PRE_WRITE: %d, _FF_WRITE_ENABLE: %d\n", 
        lc->FF_BANK_1, lc->FF_READ_ENABLE, lc->FF_PRE_WRITE, lc->_FF_WRITE_ENABLE);   

    set_memory_pages_based_on_flags(cpu);
}

uint8_t languagecard_read_C011(cpu_state *cpu, uint16_t address) {
    languagecard_state_t *lc = (languagecard_state_t *)get_module_state(cpu, MODULE_LANGCARD);

    if (DEBUG(DEBUG_LANGCARD)) printf("languagecard_read_C011 %04X FF_BANK_1: %d\n", address, lc->FF_BANK_1);
   /*  return (!lc->FF_BANK_1) << 7; */
   return (lc->FF_BANK_1 == 0) ? 0x80 : 0x00;
}

uint8_t languagecard_read_C012(cpu_state *cpu, uint16_t address) {
    languagecard_state_t *lc = (languagecard_state_t *)get_module_state(cpu, MODULE_LANGCARD);

    if (DEBUG(DEBUG_LANGCARD)) printf("languagecard_read_C012 %04X FF_READ_ENABLE: %d\n", address, lc->FF_READ_ENABLE);
    return (lc->FF_READ_ENABLE != 0) ? 0x80 : 0x00; /* << 7; */
}

void init_slot_languagecard(cpu_state *cpu, uint8_t slot) {

    fprintf(stdout, "languagecard_register_slot %d\n", slot);
    if (slot != 0) {
        fprintf(stdout, "languagecard_register_slot %d - not supported\n", slot);
        return;
    }

    languagecard_state_t *lc = new languagecard_state_t();

/** At power up, the RAM card is disabled for reading and enabled for writing.
 * the pre-write flip-flop is reset, and bank 2 is selected. 
 * the RESET .. has no effect on the RAM card configuration.
 */
    lc->FF_BANK_1 = 0;
    lc->FF_PRE_WRITE = 0;
    lc->FF_READ_ENABLE = 0;
    lc->_FF_WRITE_ENABLE = 0;

    set_module_state(cpu, MODULE_LANGCARD, lc);

    register_C0xx_memory_read_handler(0xC011, languagecard_read_C011);
    register_C0xx_memory_read_handler(0xC012, languagecard_read_C012);

    for (uint16_t i = 0xC080; i <= 0xC08F; i++) {
        register_C0xx_memory_read_handler(i, languagecard_read_C0xx);
        register_C0xx_memory_write_handler(i, languagecard_write_C0xx);
    }

    set_memory_pages_based_on_flags(cpu);
}

void reset_languagecard(cpu_state *cpu) {

}