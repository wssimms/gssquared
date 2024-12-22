#pragma once

#include <cstdint>

void memory_bus_read(uint16_t address, uint8_t value);
void memory_bus_write(uint16_t address, uint8_t value);
