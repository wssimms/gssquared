#pragma once

#include <cstdint>

uint8_t memory_bus_read(uint16_t address);
void memory_bus_write(uint16_t address, uint8_t value);
