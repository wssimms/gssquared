#pragma once

/**
 *               READ     WRITE     Bank
 * C080 - 0000 -  RAM      None      2
 * C081 - 0001 -  ROM      RAM       2
 * C082 - 0010 -  ROM      None      2
 * C083 - 0011 -  RAM      RAM       2
 * C084 - C087 - same as above
 * 
 * C088 - 1000 -  RAM      None      1
 * C089 - 1001 -  ROM      RAM       1
 * C08A - 1010 -  ROM      None      1
 * C08B - 1011 -  RAM      RAM       1
 * C08C - C08F - same as above
 * 
 * Bits 0111 - Weirdness.
 * Bit 1000 - 1 = Bank 1; 0 = Bank 2
 * 

* C011 - Read (bit 7) - bank 2 active (1) or bank 1 active (0)
* C012 - Read (bit 7) - RAM (1) or ROM (0)


 * 
 * There are 3 toggles:
 * Read RAM vs Read ROM
 * Write RAM vs No Write
 * Bank 1 vs Bank 2
 * 
 */

#include "cpu.hpp"
#include "memory.hpp"
#include "bus.hpp"

#define LANG_A3             0b00001000
#define LANG_A0A1           0b00000011

#define LANG_RAM_BANK_MASK  0b01000
#define LANG_RAM_BANK1      0b01000
#define LANG_RAM_BANK2      0b00000

#define LANG_UNUSED_BIT     0b00100

#define RAM_NONE            0b000
#define ROM_RAM             0b001
#define ROM_NONE            0b010
#define RAM_RAM             0b011


void init_languagecard(cpu_state *cpu, uint8_t slot);
void reset_languagecard(cpu_state *cpu);
