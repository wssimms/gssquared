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

#include <iostream>
#include <unistd.h>
#include <sstream>
#include "bus.hpp"

#include "memory.hpp"
#include "display/text_40x24.hpp"
#include "display/hgr_280x192.hpp"
/**
 * Process read and write to simulated IO bus for peripherals 
 * All external bus accesses are 8-bit data, 16-bit address.
 * So that is the interface here.
 *
 * */

#define C0X0_BASE 0xC000
#define C0X0_SIZE 0x100

// Table of memory read handlers
memory_read_handler C0xx_memory_read_handlers[C0X0_SIZE] = { nullptr };

// Table of memory write handlers
memory_write_handler C0xx_memory_write_handlers[C0X0_SIZE] = { nullptr };

void register_C0xx_memory_read_handler(uint16_t address, memory_read_handler handler) {
    if (address < C0X0_BASE || address >= C0X0_BASE + C0X0_SIZE) {
        return;
    }
    C0xx_memory_read_handlers[address - C0X0_BASE] = handler;
}

void register_C0xx_memory_write_handler(uint16_t address, memory_write_handler handler) {
    if (address < C0X0_BASE || address >= C0X0_BASE + C0X0_SIZE) {
        return;
    }
    C0xx_memory_write_handlers[address - C0X0_BASE] = handler;
}

uint8_t memory_bus_read(cpu_state *cpu, uint16_t address) {
    if (address >= C0X0_BASE && address < C0X0_BASE + C0X0_SIZE) {
        memory_read_handler funcptr =  C0xx_memory_read_handlers[address - C0X0_BASE];
        if (funcptr != nullptr) {
            return (*funcptr)(cpu, address);
        } else return 0xEE;
    }
    if (address >= 0xC100 && address < 0xC800) { // Slot-card firmware.
        uint8_t slot = (address / 0x100) & 0x7;
        if (cpu->C8xx_slot != slot) {
            call_C8xx_handler(cpu, slot);
        }
        return raw_memory_read(cpu, address);
    }
    /* Identifcal with what's in memory_bus_write */
    if (address == 0xCFFF) {
        cpu->C8xx_slot = 0xFF;
        for (uint8_t page = 0; page < 8; page++) {
            memory_map_page_both(cpu, page + 0xC8, cpu->main_io_4 + (page + 0x08) * 0x100, MEM_IO);
        }
        return raw_memory_read(cpu, address);
    }
    return cpu->memory->pages_read[address / GS2_PAGE_SIZE][address % GS2_PAGE_SIZE];
//    return 0xEE; /* TODO: should return a random value 'floating bus'*/
}

void memory_bus_write(cpu_state *cpu, uint16_t address, uint8_t value) {
    if (address >= 0x0400 && address <= 0x0BFF) {
        txt_memory_write(cpu, address, value);
    }
    if (address >= 0x2000 && address <= 0x5FFF) {
        hgr_memory_write(cpu, address, value);
    }
    if (address == 0xCFFF) {
        cpu->C8xx_slot = 0xFF;
        for (uint8_t page = 0; page < 8; page++) {
            memory_map_page_both(cpu, page + 0xC8, cpu->main_io_4 + (page + 0x08) * 0x100, MEM_IO);
        }
        return;
    }
    if (address >= C0X0_BASE && address < C0X0_BASE + C0X0_SIZE) {
        memory_write_handler funcptr =  C0xx_memory_write_handlers[address - C0X0_BASE];
        if (funcptr != nullptr) {
             (*funcptr)(cpu, address, value);
        } else return;
    }
}
