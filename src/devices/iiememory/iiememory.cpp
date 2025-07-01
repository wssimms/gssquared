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
#include "debug.hpp"

#include "iiememory.hpp"
#include "Module_ID.hpp"
#include "display/VideoScannerIIe.hpp"
//#include "display/display.hpp"
#include "devices/languagecard/languagecard.hpp" // to get bit flag names

/**
 * First, handling the "language card" portion or what the IIe manual calls the "Bank Switch RAM".
 * 
 * ROM: C1-CF     ROM + 0x0100 
 * ROM: D0-DF     ROM + 0x1000
 * ROM: E0-FF     ROM + 0x2000 (- 0x3FFF)
 * RAM: D0-DF     Bank 1:  RAM + 0xC000
 * RAM: D0-DF     Bank 2:  RAM + 0xD000
 * RAM: E0-FF              RAM + 0xE000
 * ALT RAM:       add 0x10000 to address 
 */

void bsr_map_memory(iiememory_state_t *lc) {

    uint32_t bankd0offset = (lc->FF_BANK_1 == 1) ? 0xC000 : 0xD000;
    uint32_t banke0offset = 0xE000;
    if (lc->f_altzp) {
        bankd0offset += 0x1'0000; // alternate bank!
        banke0offset += 0x1'0000; // alternate bank!
    }

    //uint8_t *bank = (lc->FF_BANK_1 == 1) ? lc->ram : lc->ram + 0x1000;
    uint8_t *bankd0 = lc->ram + bankd0offset;
    uint8_t *banke0 = lc->ram + banke0offset;
    uint8_t *rom = lc->mmu->get_rom_base();

    const char *bank_d = (lc->FF_BANK_1 == 1) ? "LC_BANK1" : "LC_BANK2";

    /* Map D0 - DF */
    for (int i = 0; i < 16; i++) {
        if (lc->FF_READ_ENABLE) {
            lc->mmu->map_page_read(i + 0xD0, bankd0 + (i*GS2_PAGE_SIZE), bank_d);
        } else { // reads == READ_ROM
        // TODO: this is wrong - needs to somehow know to return to ROM D0/etc wherever that may be.
        // right now hard-code to what we know about the iie rom.
            lc->mmu->map_page_read(i + 0xD0, rom + 0x1000 + (i*GS2_PAGE_SIZE), "SYS_ROM");
        }

        if (!lc->_FF_WRITE_ENABLE) {
            lc->mmu->map_page_write(i+0xD0, bankd0 + (i*GS2_PAGE_SIZE), bank_d);

        } else { // writes == WRITE_NONE - set it to the ROM and can_write = 0
            lc->mmu->map_page_write(i+0xD0, nullptr, "NONE"); // much simpler actually.. no write enable means null write pointer.
        }
    }

    /* Map E0 - FF */
    for (int i = 0; i < 32; i++) {
        if (lc->FF_READ_ENABLE) {
            lc->mmu->map_page_read(i+0xE0, banke0 + (i * GS2_PAGE_SIZE), "LC RAM");

        } else { // reads == READ_ROM
            // TODO: this is wrong - needs to somehow know to return to ROM D0/etc wherever that may be.
            lc->mmu->map_page_read(i+0xE0, rom + 0x2000 + (i * GS2_PAGE_SIZE), "SYS_ROM");
        }

        if (!lc->_FF_WRITE_ENABLE) {
            lc->mmu->map_page_write(i+0xE0, banke0 + (i * GS2_PAGE_SIZE), "LC RAM");
        } else { // writes == WRITE_NONE - set it to the ROM and can_write = 0
            lc->mmu->map_page_write(i+0xE0, nullptr, "NONE"); // much simpler actually.. no write enable means null write pointer.
        }
    }

    if (DEBUG(DEBUG_LANGCARD)) {
        lc->mmu->dump_page_table(0xD0, 0xD0);
        lc->mmu->dump_page_table(0xE0, 0xE0);
    }
}

uint8_t bsr_read_C0xx(void *context, uint16_t address) {
    iiememory_state_t *lc = (iiememory_state_t *)context;

    if (DEBUG(DEBUG_LANGCARD)) printf("languagecard read %04X ", address);

    /** Bank Select */    
    
    if (address & LANG_A3 ) {
        // 1 = any access sets Bank_1
        lc->FF_BANK_1 = 1;
    } else {
        // 0 = any access resets Bank_1
        lc->FF_BANK_1 = 0;
    }

    /** Read Enable */
    
    if (((address & LANG_A0A1) == 0) || ((address & LANG_A0A1) == 3)) {
        // 00, 11 - set READ_ENABLE
        lc->FF_READ_ENABLE = 1;
    } else {
        // 01, 10 - reset READ_ENABLE
        lc->FF_READ_ENABLE = 0;
    }

    int old_pre_write = lc->FF_PRE_WRITE;

    /* PRE_WRITE */
    if ((address & 0b00000001) == 1) {  // read 1 or 3
        // 00000001 - set PRE_WRITE
        lc->FF_PRE_WRITE = 1;
    } else {                            // read 0 or 2
        // 00000000 - reset PRE_WRITE
        lc->FF_PRE_WRITE = 0;
    }

    /** Write Enable */
    if ((old_pre_write == 1) && ((address & 0b00000001) == 1)) { // PRE_WRITE set, read 1 or 3
        // 00000000 - reset WRITE_ENABLE'
        lc->_FF_WRITE_ENABLE = 0;
    }
    if ((address & 0b00000001) == 0) { // read 0 or 2, set _WRITE_ENABLE
        // 00000001 - set WRITE_ENABLE'
        lc->_FF_WRITE_ENABLE = 1;
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

    bsr_map_memory(lc);
    return 0;
}


void bsr_write_C0xx(void *context, uint16_t address, uint8_t value) {
    iiememory_state_t *lc = (iiememory_state_t *)context;

    if (DEBUG(DEBUG_LANGCARD)) printf("languagecard write %04X value: %02X\n", address, value);
    

    /** Bank Select */

    if (address & LANG_A3 ) {
        // 1 = any access sets Bank_1 - read or write.
        lc->FF_BANK_1 = 1;
    } else {
        // 0 = any access resets Bank_1
        lc->FF_BANK_1 = 0;
    }

    /** READ_ENABLE  */
    
    if (((address & LANG_A0A1) == 0) || ((address & LANG_A0A1) == 3)) { // write 0 or 3
        // 00, 11 - set READ_ENABLE
        lc->FF_READ_ENABLE = 1;
    } else {
        // 01, 10 - reset READ_ENABLE
        lc->FF_READ_ENABLE = 0;
    }
    
    /** PRE_WRITE */ // any write, reests PRE_WRITE
    lc->FF_PRE_WRITE = 0;

    /** WRITE_ENABLE */
    if ((address & 0b00000001) == 0) { // write 0 or 2
        lc->_FF_WRITE_ENABLE = 1;
    }

    /* This means there is a feature of RAM card control
    not documented by Apple: write access to odd address in C08X
    range controls the READ ENABLE flip-flip without affecting the WRITE enable' flip-flop.
    STA $C081: no effect on write enable, disable read, bank 2 */

    if (DEBUG(DEBUG_LANGCARD)) printf("FF_BANK_1: %d, FF_READ_ENABLE: %d, FF_PRE_WRITE: %d, _FF_WRITE_ENABLE: %d\n", 
        lc->FF_BANK_1, lc->FF_READ_ENABLE, lc->FF_PRE_WRITE, lc->_FF_WRITE_ENABLE);   

    bsr_map_memory(lc);
}

uint8_t bsr_read_C011(void *context, uint16_t address) {
    iiememory_state_t *lc = (iiememory_state_t *)context;

    if (DEBUG(DEBUG_LANGCARD)) printf("languagecard_read_C011 %04X FF_BANK_1: %d\n", address, lc->FF_BANK_1);
   return (lc->FF_BANK_1 == 0) ? 0x80 : 0x00;
}

uint8_t bsr_read_C012(void *context, uint16_t address) {
    iiememory_state_t *lc = (iiememory_state_t *)context;

    if (DEBUG(DEBUG_LANGCARD)) printf("languagecard_read_C012 %04X FF_READ_ENABLE: %d\n", address, lc->FF_READ_ENABLE);
    return (lc->FF_READ_ENABLE != 0) ? 0x80 : 0x00; /* << 7; */
}


/**
 * Now the IIe-specific stuff.
 * 
 * 
 * 
 * 
 * 
 */



void update_display_flags(iiememory_state_t *iiememory_d)
{
    VideoScannerII *vs = iiememory_d->computer->video_scanner;

    //iiememory_d->s_text = vs->is_text();
    //iiememory_d->s_hires = vs->is_hires();
    //iiememory_d->s_mixed = vs->is_mixed();

    // if 80STORE is off, get page2 from display; otherwise just keep our local version, as display version is always set to page 1.
    if (!iiememory_d->f_80store) iiememory_d->s_page2 = vs->is_page_2();
}

void iiememory_debug(iiememory_state_t *iiememory_d) {
    // CX debug
    iiememory_d->mmu->dump_page_table(0xC2, 0xC3);
    /* printf("IIe Memory: m_zp: %d, m_text1_r: %d, m_text1_w: %d, m_hires1_r: %d, m_hires1_w: %d, m_all_r: %d, m_all_w: %d\n", 
        iiememory_d->m_zp, iiememory_d->m_text1_r, iiememory_d->m_text1_w, iiememory_d->m_hires1_r, iiememory_d->m_hires1_w, iiememory_d->m_all_r, iiememory_d->m_all_w); */
}

void iiememory_compose_map(iiememory_state_t *iiememory_d) {
    const char *TAG_MAIN = "MAIN";
    const char *TAG_ALT = "ALT";
    
    update_display_flags(iiememory_d);
    VideoScannerII *vs = iiememory_d->computer->video_scanner;
    
    bool n_zp = false; // this is both read and write.
    bool n_text1_r = false; // 
    bool n_text1_w = false; // 
    bool n_hires1_r = false; // 
    bool n_hires1_w = false; // 
    bool n_all_r = false; // 
    bool n_all_w = false; // 

    n_all_r = iiememory_d->f_ramrd;
    n_all_w = iiememory_d->f_ramwrt;
    n_zp = iiememory_d->f_altzp;
    if (iiememory_d->f_80store) {
//      if (iiememory_d->s_hires) {
        if (vs->is_hires()) {
            n_text1_r = iiememory_d->s_page2;
            n_text1_w = iiememory_d->s_page2;
            n_hires1_r = iiememory_d->s_page2;
            n_hires1_w = iiememory_d->s_page2;
        } else {
            n_text1_r = iiememory_d->s_page2;
            n_text1_w = iiememory_d->s_page2;
            n_hires1_r = iiememory_d->f_ramrd;
            n_hires1_w = iiememory_d->f_ramwrt;
        }
    } else { // 80STORE OFF
        n_text1_r = iiememory_d->f_ramrd;
        n_text1_w = iiememory_d->f_ramwrt;
        n_hires1_r = iiememory_d->f_ramrd;
        n_hires1_w = iiememory_d->f_ramwrt;
    }

    uint8_t *memory_base = iiememory_d->ram;

    if (n_zp != iiememory_d->m_zp) {
        // change $00, $01, $D0 - $FF
        uint32_t altoffset = n_zp ? 0x1'0000 : 0x0'0000;

        // call iiememory_map_langcard()
        iiememory_d->mmu->map_page_read(0, memory_base + altoffset + (0 * GS2_PAGE_SIZE), n_zp ? TAG_ALT : TAG_MAIN);
        iiememory_d->mmu->map_page_read(1, memory_base + altoffset + (1 * GS2_PAGE_SIZE), n_zp ? TAG_ALT : TAG_MAIN);
        iiememory_d->mmu->map_page_write(0, memory_base + altoffset + (0 * GS2_PAGE_SIZE), n_zp ? TAG_ALT : TAG_MAIN);
        iiememory_d->mmu->map_page_write(1, memory_base + altoffset + (1 * GS2_PAGE_SIZE), n_zp ? TAG_ALT : TAG_MAIN);

        bsr_map_memory(iiememory_d); // handle the 'language card' portion.
    }
    if (n_text1_r != iiememory_d->m_text1_r) {
        // change $04 - $07
        uint32_t altoffset = n_text1_r ? 0x1'0000 : 0x0'0000;
        for (int i = 0x04; i <= 0x07; i++) {
            iiememory_d->mmu->map_page_read(i, memory_base + altoffset + (i * GS2_PAGE_SIZE), n_text1_r ? TAG_ALT : TAG_MAIN);
        }
    }
    if (n_text1_w != iiememory_d->m_text1_w) {
        // change $04 - $07
        uint32_t altoffset = n_text1_w ? 0x1'0000 : 0x0'0000;
        for (int i = 0x04; i <= 0x07; i++) {
            iiememory_d->mmu->map_page_write(i, memory_base + altoffset + (i * GS2_PAGE_SIZE), n_text1_r ? TAG_ALT : TAG_MAIN);
        }
    }
    if (n_hires1_r != iiememory_d->m_hires1_r) {
        // change $20 - $3F
        uint32_t altoffset = n_hires1_r ? 0x1'0000 : 0x0'0000;
        for (int i = 0x20; i <= 0x3F; i++) {
            iiememory_d->mmu->map_page_read(i, memory_base + altoffset + (i * GS2_PAGE_SIZE), n_hires1_r ? TAG_ALT : TAG_MAIN);
        }
    }
    if (n_hires1_w != iiememory_d->m_hires1_w) {
        // change $20 - $3F
        uint32_t altoffset = n_hires1_w ? 0x1'0000 : 0x0'0000;
        for (int i = 0x20; i <= 0x3F; i++) {
            iiememory_d->mmu->map_page_write(i, memory_base + altoffset + (i * GS2_PAGE_SIZE), n_hires1_r ? TAG_ALT : TAG_MAIN);
        }
    }
    if (n_all_r != iiememory_d->m_all_r) {  
        // change $02 - $03, $08 - $1F, $40 - $BF
        uint32_t altoffset = n_all_r ? 0x1'0000 : 0x0'0000;
        for (int i = 0x02; i <= 0x03; i++) {
            iiememory_d->mmu->map_page_read(i, memory_base + altoffset + (i * GS2_PAGE_SIZE), n_all_r ? TAG_ALT : TAG_MAIN);
        }
        for (int i = 0x08; i <= 0x1F; i++) {
            iiememory_d->mmu->map_page_read(i, memory_base + altoffset + (i * GS2_PAGE_SIZE), n_all_r ? TAG_ALT : TAG_MAIN);
        }
        for (int i = 0x40; i <= 0xBF; i++) {
            iiememory_d->mmu->map_page_read(i, memory_base + altoffset + (i * GS2_PAGE_SIZE), n_all_r ? TAG_ALT : TAG_MAIN);
        }
    }
    if (n_all_w != iiememory_d->m_all_w) {
        // change $02 - $03, $08 - $1F, $40 - $BF
        uint32_t altoffset = n_all_w ? 0x1'0000 : 0x0'0000;
        for (int i = 0x02; i <= 0x03; i++) {
            iiememory_d->mmu->map_page_write(i, memory_base + altoffset + (i * GS2_PAGE_SIZE), n_all_w ? TAG_ALT : TAG_MAIN);
        }
        for (int i = 0x08; i <= 0x1F; i++) {
            iiememory_d->mmu->map_page_write(i, memory_base + altoffset + (i * GS2_PAGE_SIZE), n_all_w ? TAG_ALT : TAG_MAIN);
        }
        for (int i = 0x40; i <= 0xBF; i++) {
            iiememory_d->mmu->map_page_write(i, memory_base + altoffset + (i * GS2_PAGE_SIZE), n_all_w ? TAG_ALT : TAG_MAIN);
        }
    }

    // update the "current memory map state" flags.
    iiememory_d->m_zp = n_zp;
    iiememory_d->m_text1_r = n_text1_r;
    iiememory_d->m_text1_w = n_text1_w;
    iiememory_d->m_hires1_r = n_hires1_r;
    iiememory_d->m_hires1_w = n_hires1_w;
    iiememory_d->m_all_r = n_all_r;
    iiememory_d->m_all_w = n_all_w;

    //iiememory_debug(iiememory_d);
}

void iiememory_write_C00X(void *context, uint16_t address, uint8_t data) {
    iiememory_state_t *iiememory_d = (iiememory_state_t *)context;
    uint8_t *main_rom = iiememory_d->mmu->get_rom_base();
    VideoScannerIIe *vs = (VideoScannerIIe *)(iiememory_d->computer->video_scanner);
    
    switch (address) {

        case 0xC000: // 80STOREOFF
            iiememory_d->f_80store = false;
            break;
        case 0xC001: // 80STOREON
            iiememory_d->f_80store = true;
            break;
        case 0xC002: // RAMRDOFF
            iiememory_d->f_ramrd = false;
            break;
        case 0xC003: // RAMRDON
            iiememory_d->f_ramrd = true;
            break;
        case 0xC004: // RAMWRTOFF
            iiememory_d->f_ramwrt = false;
            break;
        case 0xC005: // RAMWRTON
            iiememory_d->f_ramwrt = true;
            break;
      
        case 0xC008: // ALTZPOFF
            iiememory_d->f_altzp = false;
            break;
        case 0xC009: // ALTZPON
            iiememory_d->f_altzp = true;
            break;
#if 0
        case 0xC00E: // ALTCHARSETOFF
            iiememory_d->f_altcharset = false;
            break;
        case 0xC00F: // ALTCHARSETON
            iiememory_d->f_altcharset = true;
            break;
#endif
    }
    iiememory_compose_map(iiememory_d);

}

uint8_t iiememory_read_C01X(void *context, uint16_t address) {
    iiememory_state_t *iiememory_d = (iiememory_state_t *)context;
    update_display_flags(iiememory_d);
    
    switch (address) {
        case 0xC011: // BSRBANK2
            return (!iiememory_d->FF_BANK_1) ? 0x80 : 0x00;

        case 0xC012: // BSRREADRAM
            return (iiememory_d->FF_READ_ENABLE) ? 0x80 : 0x00;

        case 0xC013: // RAMRD
            return (iiememory_d->f_ramrd) ? 0x80 : 0x00;

        case 0xC014: // RAMWRT
            return (iiememory_d->f_ramwrt) ? 0x80 : 0x00;

        case 0xC016: // ALTZP
            return (iiememory_d->f_altzp) ? 0x80 : 0x00;

        case 0xC017: // SLOTC3ROM
            return (iiememory_d->f_slotc3rom) ? 0x80 : 0x00;

        case 0xC018: // 80STORE
            return (iiememory_d->f_80store) ? 0x80 : 0x00;
#if 0
        // TODO: these need to go to the display device.
        case 0xC019: // VERTBLANK
            return 0x00;

        case 0xC01A: // TEXT
            return (iiememory_d->s_text) ? 0x80 : 0x00;

        case 0xC01B: // MIXED
            return (iiememory_d->s_mixed) ? 0x80 : 0x00;
#endif
        case 0xC01C: // PAGE2
            return (iiememory_d->s_page2) ? 0x80 : 0x00;
#if 0
        case 0xC01D:  // HIRES
            return (iiememory_d->s_hires) ? 0x80 : 0x00;

        case 0xC01E:  // ALTCHARSET
            return (iiememory_d->f_altcharset) ? 0x80 : 0x00;

        case 0xC01F:  // 80COL
            return (iiememory_d->f_80col) ? 0x80 : 0x00;
#endif
        default:
            return 0x00;
            break;
    }
    return 0xEE;
}

// It is necessary to take over the page2 switch from the video scanner because
// with 80store on, page2 switches between main and auxiliary memory.
uint8_t iiememory_read_C054(void *context, uint16_t address) {
    iiememory_state_t *iiememory_d = (iiememory_state_t *)context;
    VideoScannerII *vs = iiememory_d->computer->video_scanner;

    if (iiememory_d->f_80store)
        iiememory_d->s_page2 = false;
    else
        vs_bus_read_C054(vs, address);

    // recompose the memory map
    iiememory_compose_map(iiememory_d);

    return vs->get_video_byte();
}

uint8_t iiememory_read_C055(void *context, uint16_t address) {
    iiememory_state_t *iiememory_d = (iiememory_state_t *)context;
    VideoScannerII *vs = iiememory_d->computer->video_scanner;

    if (iiememory_d->f_80store)
        iiememory_d->s_page2 = true;
    else
        vs_bus_read_C055(vs, address);

    // recompose the memory map
    iiememory_compose_map(iiememory_d);

    return vs->get_video_byte();
}

void iiememory_write_C054(void *context, uint16_t address, uint8_t data) {
    iiememory_read_C054(context, address);
}

void iiememory_write_C055(void *context, uint16_t address, uint8_t data) {
    iiememory_read_C055(context, address);
}

#if 0
uint8_t iiememory_read_display(void *context, uint16_t address) {
    iiememory_state_t *iiememory_d = (iiememory_state_t *)context;
    VideoScannerII *vs = iiememory_d->computer->video_scanner;

    // call down to the old display device handlers for these..
    uint8_t retval = 0x00;
    switch (address) {
        case 0xC050: // TEXTOFF
            retval = vs_bus_read_C050(vs, address);
            break;
        case 0xC051: // TEXTON
            retval = vs_bus_read_C051(vs, address);
            break;
        case 0xC052: // MIXEDOFF
            retval = vs_bus_read_C052(vs, address);
            break;
        case 0xC053: // MIXEDON
            retval = vs_bus_read_C053(vs, address);
            break;
        case 0xC054: // PAGE2OFF
            if (!iiememory_d->f_80store) retval = vs_bus_read_C054(vs, address);
            else iiememory_d->s_page2 = false;
            break;
        case 0xC055: // PAGE2ON
            if (!iiememory_d->f_80store) retval = vs_bus_read_C055(vs, address);
            else iiememory_d->s_page2 = true;
            break;
        case 0xC056: // HIRESOFF
            retval = vs_bus_read_C056(vs, address);
            break;
        case 0xC057: // HIRESON
            retval = vs_bus_read_C057(vs, address);
            break;
    }
    // recompose the memory map
    iiememory_compose_map(iiememory_d);
    return retval;
}

// write - do same as read but disregard return value.
void iiememory_write_display(void *context, uint16_t address, uint8_t data) {
    iiememory_state_t *iiememory_d = (iiememory_state_t *)context;
    iiememory_read_display(context, address);
}
#endif

/*
 * When you initiate a reset, hardware in the Apple IIe sets the memory-controlling soft switches to normal: 
 * main board RAM and ROM are enabled; if there is an 80 column card in the aux slot, expansion slot 3 is allocated 
 * to the built-in 80 column firmware. 
 * auxiliary ram is disabled and the BSR is set up to read ROM and write RAM, bank 2. (hardware)
*/

void reset_iiememory(iiememory_state_t *lc) {

    // in a IIe RESET does affect the memory map.
    lc->FF_BANK_1 = 0;
    lc->FF_PRE_WRITE = 0;
    lc->FF_READ_ENABLE = 0;
    lc->_FF_WRITE_ENABLE = 0;

    lc->f_80store = false;
    lc->f_ramrd = false;
    lc->f_ramwrt = false;
    lc->f_altzp = false;
    lc->f_slotc3rom = false;
    //lc->f_80col = false;

    bsr_map_memory(lc);
    iiememory_compose_map(lc);
}

void init_iiememory(computer_t *computer, SlotType_t slot) {
    
    iiememory_state_t *iiememory_d = new iiememory_state_t;
    iiememory_d->computer = computer;
    iiememory_d->mmu = computer->mmu;
    iiememory_d->ram = computer->mmu->get_memory_base();
    
    computer->set_module_state(MODULE_IIEMEMORY, iiememory_d);
    
    for (int i = 0xC000; i <= 0xC009; i++) {
        if (i == 0xC006 || i == 0xC007) continue; // INTCXROM is handled by the MMU.
        if (i == 0xC00C || i == 0xC00D) continue; // 80COL is handled by the display device.
        computer->mmu->set_C0XX_write_handler(i, { iiememory_write_C00X, iiememory_d });
    }

    computer->mmu->set_C0XX_read_handler(0xC011, { iiememory_read_C01X, iiememory_d });
    computer->mmu->set_C0XX_read_handler(0xC012, { iiememory_read_C01X, iiememory_d });
    computer->mmu->set_C0XX_read_handler(0xC013, { iiememory_read_C01X, iiememory_d });
    computer->mmu->set_C0XX_read_handler(0xC014, { iiememory_read_C01X, iiememory_d });
    // C015 (INTCXROM) is handled by the MMU.
    computer->mmu->set_C0XX_read_handler(0xC016, { iiememory_read_C01X, iiememory_d });
    computer->mmu->set_C0XX_read_handler(0xC017, { iiememory_read_C01X, iiememory_d });
    computer->mmu->set_C0XX_read_handler(0xC018, { iiememory_read_C01X, iiememory_d });
    // C019, C01A, C01B are handled by the video scanner
    computer->mmu->set_C0XX_read_handler(0xC01C, { iiememory_read_C01X, iiememory_d });
    // C01D, C01E, C01F are handled by the video scanner

    /*
    // Override the display device handlers for these..
    for (uint16_t i = 0xC050; i <= 0xC057; i++) {
        computer->mmu->set_C0XX_read_handler(i, { iiememory_read_display, iiememory_d });
        computer->mmu->set_C0XX_write_handler(i, { iiememory_write_display, iiememory_d });
    }
    */

    // It is necessary to take over the page2 switch from the video scanner because
    // with 80store on, page2 switches between main and auxiliary memory.
    computer->mmu->set_C0XX_read_handler(0xC054, { iiememory_read_C054, iiememory_d });
    computer->mmu->set_C0XX_read_handler(0xC055, { iiememory_read_C055, iiememory_d });
    computer->mmu->set_C0XX_write_handler(0xC054, { iiememory_write_C054, iiememory_d });
    computer->mmu->set_C0XX_write_handler(0xC055, { iiememory_write_C055, iiememory_d });

    /**
     * set up the "BSR" state.0
     */
    /** At power up, the RAM card is disabled for reading and enabled for writing.
    * the pre-write flip-flop is reset, and bank 2 is selected. 
    * the RESET .. has no effect on the RAM card configuration.
    */
    iiememory_d->FF_BANK_1 = 0;
    iiememory_d->FF_PRE_WRITE = 0;
    iiememory_d->FF_READ_ENABLE = 0;
    iiememory_d->_FF_WRITE_ENABLE = 0;

    iiememory_d->mmu->set_C0XX_read_handler(0xC011, { bsr_read_C011, iiememory_d });
    iiememory_d->mmu->set_C0XX_read_handler(0xC012, { bsr_read_C012, iiememory_d });

    for (uint16_t i = 0xC080; i <= 0xC08F; i++) {
        iiememory_d->mmu->set_C0XX_read_handler(i, { bsr_read_C0xx, iiememory_d });
        iiememory_d->mmu->set_C0XX_write_handler(i, { bsr_write_C0xx, iiememory_d });
    }

    bsr_map_memory(iiememory_d);

    // need to handle reset here.
    computer->register_reset_handler(
        [iiememory_d]() {
            reset_iiememory(iiememory_d);
            return true;
        });
}

