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
#include <iomanip>
#include <cassert>
#include "cpu.hpp"
#include "gs2.hpp"
#include "debug.hpp"
#include "clock.hpp"
#include "bus.hpp"
#include "memory.hpp"

/* Read and write to memory-mapped memory */

/**
 * Memory is mapped per CPU, in pages of 256 bytes.
 * That's how the Apple II+ and a bunch of other stuff works.
 * 
 * We maintain separate pointer tables for read and write, so that reads
 * can go to one memory, and writes to another, which is a requirement
 * for the Language Card.
 */

/**
 * Also the memory_write and raw_memory_write functions need to obey the
 * can_write flag.
 */

// Raw. Do not trigger cycles or do the IO bus stuff
uint8_t raw_memory_read(cpu_state *cpu, uint16_t address) {
    return cpu->memory->pages_read[address / GS2_PAGE_SIZE][address % GS2_PAGE_SIZE];
}

// no writable check here, do it higher up - this needs to be able to write to 
// memory block no matter what.
void raw_memory_write(cpu_state *cpu, uint16_t address, uint8_t value) {
    cpu->memory->pages_write[address / GS2_PAGE_SIZE][address % GS2_PAGE_SIZE] = value;
}

void raw_memory_write_word(cpu_state *cpu, uint16_t address, uint16_t value) {
    raw_memory_write(cpu, address, value & 0xFF);
    raw_memory_write(cpu, address + 1, value >> 8);
}

uint8_t read_memory(cpu_state *cpu, uint16_t address) {
    incr_cycles(cpu);
    uint8_t typ = cpu->memory->page_info[address / GS2_PAGE_SIZE].type;
    if (typ == MEM_ROM || typ == MEM_RAM) {
        return cpu->memory->pages_read[address / GS2_PAGE_SIZE][address % GS2_PAGE_SIZE];
    }
    // IO - call the memory bus dispatcher thingy.
    return memory_bus_read(cpu, address);
}

void write_memory(cpu_state *cpu, uint16_t address, uint8_t value) {
    uint16_t page = address / GS2_PAGE_SIZE;
    uint16_t offset = address % GS2_PAGE_SIZE;

    assert(page < 0x100);
    assert(offset < GS2_PAGE_SIZE);

    incr_cycles(cpu);
    uint8_t typ = cpu->memory->page_info[page].type;

    // if IO, only call the memory bus dispatcher thingy
    if (typ == MEM_IO) {
        memory_bus_write(cpu, address, value);
        return;
    }
    if (address == 0xCFFF) { // maybe we should always mark these as MEM_IO
        memory_bus_write(cpu, address, value);
        return;
    }
    
    // ROM and "write protected RAM", don't write
    if (!cpu->memory->page_info[page].can_write) {
        return;
    }

    // if RAM, write
    cpu->memory->pages_write[page][offset] = value;
    memory_bus_write(cpu, address, value); // catch writes to video memory.
}

uint8_t read_byte(cpu_state *cpu, uint16_t address) {
    uint8_t value = read_memory(cpu, address);
    //memory_bus_read(address, value);
    return value;
}

uint16_t read_word(cpu_state *cpu, uint16_t address) {
    return read_byte(cpu, address) | (read_byte(cpu, address + 1) << 8);
}

uint16_t read_word_from_pc(cpu_state *cpu) {
    uint16_t value = read_byte(cpu, cpu->pc) | (read_byte(cpu, cpu->pc + 1) << 8);
    cpu->pc += 2;
    return value;
}

void write_byte(cpu_state *cpu, uint16_t address, uint8_t value) {
    write_memory(cpu, address, value);
    //memory_bus_write(address, value);
}

void write_word(cpu_state *cpu, uint16_t address, uint16_t value) {
    write_byte(cpu, address, value & 0xFF);
    //memory_bus_write(address, value);
    write_byte(cpu, address + 1, value >> 8);
    //memory_bus_write(address + 1, value >> 8);
}

void store_byte(cpu_state *cpu, uint16_t address, uint8_t value) {
    write_byte(cpu, address, value);
}

void store_word(cpu_state *cpu, uint16_t address, uint16_t value) {
    store_byte(cpu, address, value & 0xFF);
    store_byte(cpu, address + 1, value >> 8);
}

uint8_t read_byte_from_pc(cpu_state *cpu) {
    uint8_t opcode = read_byte(cpu, cpu->pc);
    cpu->pc++;
    return opcode;
}

void memory_map_page_both(cpu_state *cpu, uint16_t page, uint8_t *data, memory_type type) {
    if (page > 0xFF) { // TODO: this is a bounds check hack for now. Must read number of pages somewhere.
        return;
    }
    cpu->memory->page_info[page].type = type;
    cpu->memory->pages_read[page] = data;
    cpu->memory->pages_write[page] = data;
}

void register_C8xx_handler(cpu_state *cpu, SlotType_t slot, void (*handler)(cpu_state *cpu, SlotType_t slot)) {
    cpu->C8xx_handlers[slot] = handler;
}

void call_C8xx_handler(cpu_state *cpu, SlotType_t slot) {
    if (cpu->C8xx_handlers[slot] != nullptr) {
        cpu->C8xx_handlers[slot](cpu, slot);
    }
    cpu->C8xx_slot = slot;
}