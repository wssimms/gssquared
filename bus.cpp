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
        return raw_memory_read(cpu, address);
    }
    return 0xEE; /* TODO: should return a random value 'floating bus'*/
}

void memory_bus_write(cpu_state *cpu, uint16_t address, uint8_t value) {
    if (address >= 0x0400 && address <= 0x0BFF) {
        txt_memory_write(address, value);
    }
    if (address >= 0x2000 && address <= 0x5FFF) {
        hgr_memory_write(address, value);
    }
    if (address >= C0X0_BASE && address < C0X0_BASE + C0X0_SIZE) {
        memory_write_handler funcptr =  C0xx_memory_write_handlers[address - C0X0_BASE];
        if (funcptr != nullptr) {
             (*funcptr)(cpu, address, value);
        } else return;
    }
}
