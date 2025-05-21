#pragma once

#include <stdint.h>
#include <stddef.h>

#include "debugger/trace.hpp"
#include "debugger/trace_opcodes.hpp"


enum address_mode {
    NONE = 0,
    ACC = 1,
    ABS = 2,
    ABS_X = 3,
    ABS_Y = 4,
    IMM = 5,
    IMP = 6,
    INDIR = 7,
    INDEX_INDIR = 8,
    INDIR_INDEX = 9,
    REL = 10,
    ZP = 11,
    ZP_X = 12,
    ZP_Y = 13,
};

struct disasm_entry {
    const char *opcode;
    address_mode mode;
};

struct address_mode_entry {
    const char *format;
    uint8_t size; // how many bytes is the instruction
};

extern const disasm_entry disasm_table[256];
extern const address_mode_entry address_mode_formats[];
