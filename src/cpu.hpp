/*
 *   Copyright (c) 2025 Jawaid Bazyar

 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.

 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.

 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>
#include <stddef.h>
#include <cstdint>

#include <SDL3/SDL.h>

#include "memoryspecs.hpp"
#include "clock.hpp"
#include "devices.hpp"
#include "SlotData.hpp"
#include "debugger/trace.hpp"
#include "mmus/mmu_ii.hpp"
#include "Module_ID.hpp"

#define MAX_CPUS 1

#define BRK_VECTOR 0xFFFE
#define IRQ_VECTOR 0xFFFE
#define NMI_VECTOR 0xFFFA
#define RESET_VECTOR 0xFFFC

enum processor_type {
    PROCESSOR_6502 = 0,
    PROCESSOR_65C02,
    NUM_PROCESSOR_TYPES
};

enum execution_modes_t {
    EXEC_NORMAL = 0,
    EXEC_STEP_INTO,
    EXEC_STEP_OVER
};

// TODO: deal with endianness here.

struct addr_t {
    union {
        struct {
            uint8_t al;
            uint8_t ah;
        };
        uint16_t a;
    };
};

// a couple forward declarations
struct cpu_state;
class Mounts;
struct debug_window_t;

typedef int (*execute_next_fn)(cpu_state *cpu);

struct processor_model {
    const char* name;
    execute_next_fn execute_next;
};

extern processor_model processor_models[NUM_PROCESSOR_TYPES];

namespace cpu_6502 {
    extern int execute_next(cpu_state *cpu);
}
namespace cpu_65c02 {
    extern int execute_next(cpu_state *cpu);
}

struct rom_data;

struct cpu_state {
    union {
        struct {
            #if SDL_BYTEORDER == SDL_LIL_ENDIAN
                uint16_t pc;  /* Program Counter - lower 16 bits of the 24-bit program counter */
                uint8_t pb;   /* Program Bank - upper 8 bits of the 24-bit program counter */
                uint8_t unused; /* Padding to align with 32-bit full_pc */
            #elif SDL_BYTEORDER == SDL_BIG_ENDIAN
                uint8_t unused; /* Padding to align with 32-bit full_pc */
                uint8_t pb;   /* Program Bank - upper 8 bits of the 24-bit program counter */
                uint16_t pc;  /* Program Counter - lower 16 bits of the 24-bit program counter */
            #else
                #error "Endianness not supported"
            #endif
        };
        uint32_t full_pc; /* Full 24-bit program counter (with 8 unused bits) */
    };

    uint8_t db;   /* Data Bank register */
    uint16_t sp;  /* Stack Pointer */
    union {
        struct {
            #if SDL_BYTEORDER == SDL_LIL_ENDIAN
                uint8_t a_lo;  /* Lower 8 bits of Accumulator */
                uint8_t a_hi;  /* Upper 8 bits of Accumulator */
            #elif SDL_BYTEORDER == SDL_BIG_ENDIAN
                uint8_t a_hi;  /* Upper 8 bits of Accumulator */
                uint8_t a_lo;  /* Lower 8 bits of Accumulator */
            #else
                #error "Endianness not supported"
            #endif
        };
        uint16_t a;   /* Full 16-bit Accumulator */
    };
    union {
        struct {
            #if SDL_BYTEORDER == SDL_LIL_ENDIAN
                uint8_t x_lo;  /* Lower 8 bits of X Index Register */
                uint8_t x_hi;  /* Upper 8 bits of X Index Register */
            #elif SDL_BYTEORDER == SDL_BIG_ENDIAN
                uint8_t x_hi;  /* Upper 8 bits of X Index Register */
                uint8_t x_lo;  /* Lower 8 bits of X Index Register */
            #else
                #error "Endianness not supported"
            #endif
        };
        uint16_t x;   /* Full 16-bit X Index Register */
    };
    union {
        struct {
            #if SDL_BYTEORDER == SDL_LIL_ENDIAN
                uint8_t y_lo;  /* Lower 8 bits of Y Index Register */
                uint8_t y_hi;  /* Upper 8 bits of Y Index Register */
            #elif SDL_BYTEORDER == SDL_BIG_ENDIAN
                uint8_t y_hi;  /* Upper 8 bits of Y Index Register */
                uint8_t y_lo;  /* Lower 8 bits of Y Index Register */
            #else
                #error "Endianness not supported"
            #endif
        };
        uint16_t y;   /* Full 16-bit Y Index Register */
    };
    union {
        struct {
            #if SDL_BYTEORDER == SDL_LIL_ENDIAN
                uint8_t d_lo;  /* Lower 8 bits of Direct Page Register */
                uint8_t d_hi;  /* Upper 8 bits of Direct Page Register */
            #elif SDL_BYTEORDER == SDL_BIG_ENDIAN
                uint8_t d_hi;  /* Upper 8 bits of Direct Page Register */
                uint8_t d_lo;  /* Lower 8 bits of Y Index Register */
            #else
                #error "Endianness not supported"
            #endif
        };
        uint16_t d;   /* Full 16-bit Y Index Register */
    };
    union {
        struct {
            uint8_t C : 1;  /* Carry Flag */
            uint8_t Z : 1;  /* Zero Flag */
            uint8_t I : 1;  /* Interrupt Disable Flag */
            uint8_t D : 1;  /* Decimal Mode Flag */
            uint8_t B : 1;  /* Break Command Flag */
            uint8_t _unused : 1;  /* Unused bit */
            uint8_t V : 1;  /* Overflow Flag */
            uint8_t N : 1;  /* Negative Flag */
        };
        uint8_t p;  /* Processor Status Register */
    };
    uint8_t halt = 0; /* == 1 is HLT instruction halt; == 2 is user halt */
    uint64_t cycles; /* Number of cycles since reset */

    rom_data *rd;

    uint64_t irq_asserted = 0; /** bits 0-7 correspond to slot IRQ lines slots 0-7. */

    MMU_II *mmu = nullptr;

    uint64_t last_tick;
    uint64_t next_tick;
    uint64_t clock_slip = 0;
    uint64_t clock_busy = 0;
    uint64_t clock_sleep = 0;
    uint64_t cycle_duration_ns;
    uint64_t HZ_RATE;
    clock_mode_t clock_mode = CLOCK_FREE_RUN;
    float e_mhz = 0;
    
    execute_next_fn execute_next;

    void *module_store[MODULE_NUM_MODULES];
    SlotData *slot_store[NUM_SLOTS];

    /* Tracing & Debug */
    /* These are CPU controls, leave them here */
    bool trace = false;
    system_trace_buffer *trace_buffer;
    system_trace_entry_t trace_entry;
    execution_modes_t execution_mode = EXEC_NORMAL;
    uint64_t instructions_left = 0;

    void init();
    ~cpu_state();
    
    void set_processor(int processor_type);
    void reset();
    
    void set_mmu(MMU *mmu) { this->mmu = (MMU_II *) mmu; }

    inline uint8_t read_byte(uint16_t address) {
        incr_cycles(this);
        uint8_t value = mmu->read(address);
        return value;
    }

    inline uint16_t read_word(uint16_t address) {
        return read_byte(address) | (read_byte(address + 1) << 8);
    }

    inline uint16_t read_word_from_pc() {
        // make sure this is read lo-byte first.
        addr_t ad;
        ad.al = read_byte(pc);
        ad.ah = read_byte(pc + 1);
        pc += 2;
        return ad.a;
    }

    inline void write_byte( uint16_t address, uint8_t value) {
        incr_cycles(this);
        mmu->write(address, value);
    }

    inline void write_word(uint16_t address, uint16_t value) {
        write_byte(address, value & 0xFF);
        write_byte(address + 1, value >> 8);
    }

    inline uint8_t read_byte_from_pc() {
        uint8_t opcode = read_byte(pc);
        pc++;
        return opcode;
    }
};

#define HLT_INSTRUCTION 1
#define HLT_USER 2

#define FLAG_C        0b00000001 /* 0x01 */
#define FLAG_Z        0b00000010 /* 0x02 */
#define FLAG_I        0b00000100 /* 0x04 */
#define FLAG_D        0b00001000 /* 0x08 */
#define FLAG_B        0b00010000 /* 0x10 */
#define FLAG_UNUSED   0b00100000 /* 0x20 */
#define FLAG_V        0b01000000 /* 0x40 */
#define FLAG_N        0b10000000 /* 0x80 */

extern struct cpu_state *CPUs[MAX_CPUS];

void toggle_clock_mode(cpu_state *cpu);

void set_clock_mode(cpu_state *cpu, clock_mode_t mode);

const char* processor_get_name(int processor_type);

void *get_module_state(cpu_state *cpu, module_id_t module_id);
void set_module_state(cpu_state *cpu, module_id_t module_id, void *state);

SlotData *get_slot_state(cpu_state *cpu, SlotType_t slot);
SlotData *get_slot_state_by_id(cpu_state *cpu, device_id id);
void set_slot_state(cpu_state *cpu, SlotType_t slot, SlotData *state);

void set_slot_irq(cpu_state *cpu, uint8_t slot, bool irq);
