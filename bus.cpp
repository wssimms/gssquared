#include <iostream>
#include <unistd.h>
#include <sstream>
#include "bus.hpp"

#include "display/text_40x24.hpp"
#include "devices/keyboard.hpp"

/**
 * Process read and write to simulated IO bus for peripherals 
 * All external bus accesses are 8-bit data, 16-bit address.
 * So that is the interface here.
 *
 * */

uint8_t memory_bus_read(uint16_t address) {
    if (address == KB_LATCH_ADDRESS) {
        return kb_memory_read(address);
    }
    if (address == KB_CLEAR_LATCH_ADDRESS) {
        return kb_memory_read(address);
    }
    return 0xEE; /* TODO: should return a random value 'floating bus'*/
}

void memory_bus_write(uint16_t address, uint8_t value) {
    if (address >= 0x0400 && address <= 0x07FF) {
        txt_memory_write(address, value);
    }
    if (address == KB_LATCH_ADDRESS) {
        kb_memory_write(address, value);
    }
    if (address == KB_CLEAR_LATCH_ADDRESS) {
        kb_memory_write(address, value);
    }
}
