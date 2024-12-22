#include <iostream>
#include <unistd.h>
#include <sstream>
#include "bus.hpp"

/**
 * Process read and write to simulated IO bus for peripherals 
 * All external bus accesses are 8-bit data, 16-bit address.
 * So that is the interface here.
 *
 * */

void memory_bus_read(uint16_t address, uint8_t value) {

}

void memory_bus_write(uint16_t address, uint8_t value) {

}