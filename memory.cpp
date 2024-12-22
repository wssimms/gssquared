#include <iostream>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include "cpu.hpp"
#include "gs2.hpp"
#include "debug.hpp"
#include "clock.hpp"
#include "bus.hpp"

/* Read and write to memory-mapped memory */

// Raw. Do not trigger cycles or do the IO bus stuff
 uint8_t raw_memory_read(cpu_state *cpu, uint16_t address) {
    return cpu->memory->pages[address / GS2_PAGE_SIZE]->data[address % GS2_PAGE_SIZE];
}

 void raw_memory_write(cpu_state *cpu, uint16_t address, uint8_t value) {
    cpu->memory->pages[address / GS2_PAGE_SIZE]->data[address % GS2_PAGE_SIZE] = value;
}

 void raw_memory_write_word(cpu_state *cpu, uint16_t address, uint16_t value) {
    raw_memory_write(cpu, address, value & 0xFF);
    raw_memory_write(cpu, address + 1, value >> 8);
}

uint8_t read_memory(cpu_state *cpu, uint16_t address) {
    incr_cycles(cpu);
    return cpu->memory->pages[address / GS2_PAGE_SIZE]->data[address % GS2_PAGE_SIZE];
}

void write_memory(cpu_state *cpu, uint16_t address, uint8_t value) {
    incr_cycles(cpu);
    cpu->memory->pages[address / GS2_PAGE_SIZE]->data[address % GS2_PAGE_SIZE] = value;
}

 uint8_t read_byte(cpu_state *cpu, uint16_t address) {
    uint8_t value = read_memory(cpu, address);
    memory_bus_read(address, value);
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
    memory_bus_write(address, value);
}

 void write_word(cpu_state *cpu, uint16_t address, uint16_t value) {
    write_byte(cpu, address, value & 0xFF);
    memory_bus_write(address, value);
    write_byte(cpu, address + 1, value >> 8);
    memory_bus_write(address + 1, value >> 8);
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