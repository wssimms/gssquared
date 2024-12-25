#include <cstdio>
#include "../types.hpp"

// Software should be able to:
// Read keyboard from register at $C000.
// Write to the keyboard clear latch at $C010.

uint8_t kb_key_strobe = 0xC1;

void kb_key_pressed(uint8_t key) {
    kb_key_strobe = key | 0x80;
}

void kb_clear_strobe() {
    kb_key_strobe = kb_key_strobe & 0x7F;
}

uint8_t kb_memory_read(uint16_t address) {
    //fprintf(stderr, "kb_memory_read %04X\n", address);
    if (address == 0xC000) {
        uint8_t key = kb_key_strobe;
        return key;
    }
    if (address == 0xC010) {
        // Clear the keyboard latch
        kb_clear_strobe();
        return 0xEE;
    }
    return 0xEE;
}

void kb_memory_write(uint16_t address, uint8_t value) {
    if (address == 0xC010) {
        // Clear the keyboard latch
        kb_clear_strobe();
    }
}