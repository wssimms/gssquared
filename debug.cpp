#include <iostream>
#include <sstream>
#include <iomanip>

#include "gs2.hpp"
#include "memory.hpp"
#include "debug.hpp"

uint64_t debug_level = DEBUG_BOOT_FLAG ;

inline void print_hex(uint8_t value) {
    std::cout << std::hex << std::uppercase << static_cast<int>(value) << std::dec;
}

std::string int_to_hex(uint8_t value) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0') << static_cast<int>(value);
    return ss.str();
}

std::string int_to_hex(uint16_t value) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << static_cast<int>(value);
    return ss.str();
}

void debug_dump_memory(cpu_state *cpu, uint32_t start, uint32_t end) {
    for (uint32_t i = start; i <= end; i++) {
        std::cout << int_to_hex((uint16_t)i) << ": " << int_to_hex(raw_memory_read(cpu, i)) << " ";
        if ((i % 16) == 0xF) std::cout << std::endl;
    }
    std::cout << std::endl;
}

void debug_set_level(uint64_t level) {
    debug_level = level;
};
