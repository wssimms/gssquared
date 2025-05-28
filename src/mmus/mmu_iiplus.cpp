#include "mmu_iiplus.hpp"

/**
 * Sets base memory map without any specificity for various devices.
 */
void MMU_IIPlus::init_map() {
    // Apple II Plus - first 48K is RAM, 0000 - BFFF
     for (int i = 0; i < ram_pages; i++) {
        map_page_both(i, main_ram_64 + i * GS2_PAGE_SIZE, M_RAM, 1, 1);

    }
    // Next, 4K of I/O area, C000 - CFFF
    /* for (int i = 0; i < (IO_KB / GS2_PAGE_SIZE); i++) {
        map_page_both(i + 0xC0, main_io_4 + i * GS2_PAGE_SIZE, M_IO, 1, 1);
       
    } */
    // Then 12K of ROM. D000 - FFFF
    for (int i = 0; i < (ROM_KB / GS2_PAGE_SIZE); i++) {
        map_page_read_only(i + 0xD0, main_rom_D0 + i * GS2_PAGE_SIZE, M_ROM);

    }   
}

MMU_IIPlus::MMU_IIPlus(int ram_pages) : MMU(256) {
    ram_pages = ram_pages;
    main_ram_64 = new uint8_t[ram_pages * GS2_PAGE_SIZE];
    main_io_4 = new uint8_t[IO_KB];
    main_rom_D0 = new uint8_t[ROM_KB];

    // initialize memory map
    init_map();
    set_default_C8xx_map();
}

MMU_IIPlus::~MMU_IIPlus() {
    // free up memory areas.
    delete[] main_ram_64;
    delete[] main_io_4;
    delete[] main_rom_D0;
}

/**
 * sets default C800 - CFFF map.
 * Called with location CFFF hit, or, on reset.
 */
void MMU_IIPlus::set_default_C8xx_map() {
    C8xx_slot = 0xFF;
    for (uint8_t page = 0; page < 8; page++) {
        map_page_read_only(page + 0xC8, main_io_4 + (page + 0x08) * 0x100, M_IO);
    }
}

/**
 * read
 * Applies some of the Apple II built-in memory map logic. 
 * C000 - C0FF. 
 * C800 - CFFF.
 * CFFF to reset the C8xx map.
 */
uint8_t MMU_IIPlus::read(uint32_t address) {
    uint8_t bank = address >> 12;
    page_t page = address >> 8;
    if (bank == 0xC) {
        if (page == 0xC0) {
            read_handler_t funcptr =  C0xx_memory_read_handlers[address & 0xFF];
            if (funcptr.read != nullptr) {
                return (*funcptr.read)(funcptr.context, address);
            }
            return floating_bus_read();
        }

        /**
         * For any access in C100 - C7FF, we:
         * call any registered C8xx handler so the device can map in its own scheme from C800 - CFFF.
         */

        if (page < 0xC8) { // it's not C0, but it's less than C8 - Slot-card firmware.
            uint8_t slot = page & 0x7; // slot number is just the lower digit of page
            if (C8xx_slot != slot) call_C8xx_handler((SlotType_t)slot);
        } else 
            if (address == 0xCFFF) set_default_C8xx_map(); // When CFFF is read, reset the C8xx map to default, then execute the underlying read of CFFF.
    }
    return MMU::read(address);
}


void MMU_IIPlus::write(uint32_t address, uint8_t value) {
    uint8_t bank = address >> 12;
    page_t page = address >> 8;

    if (bank == 0xC) {
        if (page == 0xC0) {
            write_handler_t funcptr =  C0xx_memory_write_handlers[address & 0xFF];
            if (funcptr.write != nullptr) {
                (*funcptr.write)(funcptr.context, address, value);
            }
            return;
        }

        /** Handle the C800-CFFF mapping  */
        if (page < 0xC8) { // it's not C0, and less than C8 - Slot-card firmware area.
            uint8_t slot = (address / 0x100) & 0x7;
            if (C8xx_slot != slot) call_C8xx_handler((SlotType_t)slot);            
        } else if (address == 0xCFFF) set_default_C8xx_map();
    }
    MMU::write(address, value);
}

void MMU_IIPlus::set_C8xx_handler(SlotType_t slot, void (*handler)(void *context, SlotType_t slot), void *context) {
    C8xx_handlers[slot].handler = handler;
    C8xx_handlers[slot].context = context;
}

void MMU_IIPlus::call_C8xx_handler(SlotType_t slot) {
    if (C8xx_handlers[slot].handler != nullptr) {
        C8xx_handlers[slot].handler(C8xx_handlers[slot].context, slot);
    }
    C8xx_slot = slot;
}

void MMU_IIPlus::set_C0XX_read_handler(uint16_t address, read_handler_t handler) {
    if (address < C0X0_BASE || address >= C0X0_BASE + C0X0_SIZE) {
        return;
    }
    C0xx_memory_read_handlers[address - C0X0_BASE] = handler;
}

void MMU_IIPlus::set_C0XX_write_handler(uint16_t address, write_handler_t handler) {
    if (address < C0X0_BASE || address >= C0X0_BASE + C0X0_SIZE) {
        return;
    }
    C0xx_memory_write_handlers[address - C0X0_BASE] = handler;
}

void MMU_IIPlus::set_slot_rom(SlotType_t slot, uint8_t *rom) {
    if (slot < 1 || slot >= 8) {
        return;
    }
    map_page_read_only(0xC0 + slot, rom, M_ROM);
}

void MMU_IIPlus::reset() {
    set_default_C8xx_map();
}

void MMU_IIPlus::dump_C0XX_handlers() {
    printf("C0XX handlers:\n");
    for (int i = 0; i < C0X0_SIZE; i++) {
        if (C0xx_memory_read_handlers[i].read != nullptr) {
            printf("C0%02X: %p\n", i, C0xx_memory_read_handlers[i].read);
        }
    }
}