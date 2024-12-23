#pragma once

#include <cstdint>

#include "gs2.hpp"
#include "memoryspecs.hpp"
#include "types.hpp"
#define MAX_CPUS 1

struct memory_page {
    uint8_t data[GS2_PAGE_SIZE];
};

enum memory_type {
    MEM_RAM,
    MEM_ROM,
    MEM_IO,
};

struct memory_page_info {
    uint16_t start_address;
    uint16_t end_address;
    uint8_t can_read;
    uint8_t can_write;
    memory_type type;
};

struct memory_map {
    memory_page_info page_info[RAM_SIZE / GS2_PAGE_SIZE];
    memory_page *pages[RAM_SIZE / GS2_PAGE_SIZE];
};

struct cpu_state {
    uint64_t boot_time; 
    union {
        struct {
            #if defined(__LITTLE_ENDIAN__)
                uint16_t pc;  /* Program Counter - lower 16 bits of the 24-bit program counter */
                uint8_t pb;   /* Program Bank - upper 8 bits of the 24-bit program counter */
                uint8_t unused; /* Padding to align with 32-bit full_pc */
            #elif defined(__BIG_ENDIAN__)
                uint8_t unused; /* Padding to align with 32-bit full_pc */
                uint8_t pb;   /* Program Bank - upper 8 bits of the 24-bit program counter */
                uint16_t pc;  /* Program Counter - lower 16 bits of the 24-bit program counter */
            #else
                #error "Endianness not defined. Please define __LITTLE_ENDIAN__ or __BIG_ENDIAN__"
            #endif
        };
        uint32_t full_pc; /* Full 24-bit program counter (with 8 unused bits) */
    };

    uint8_t db;   /* Data Bank register */
    uint16_t sp;  /* Stack Pointer */
    union {
        struct {
            #if defined(__LITTLE_ENDIAN__)
                uint8_t a_lo;  /* Lower 8 bits of Accumulator */
                uint8_t a_hi;  /* Upper 8 bits of Accumulator */
            #elif defined(__BIG_ENDIAN__)
                uint8_t a_hi;  /* Upper 8 bits of Accumulator */
                uint8_t a_lo;  /* Lower 8 bits of Accumulator */
            #else
                #error "Endianness not defined. Please define __LITTLE_ENDIAN__ or __BIG_ENDIAN__"
            #endif
        };
        uint16_t a;   /* Full 16-bit Accumulator */
    };
    union {
        struct {
            #if defined(__LITTLE_ENDIAN__)
                uint8_t x_lo;  /* Lower 8 bits of X Index Register */
                uint8_t x_hi;  /* Upper 8 bits of X Index Register */
            #elif defined(__BIG_ENDIAN__)
                uint8_t x_hi;  /* Upper 8 bits of X Index Register */
                uint8_t x_lo;  /* Lower 8 bits of X Index Register */
            #else
                #error "Endianness not defined. Please define __LITTLE_ENDIAN__ or __BIG_ENDIAN__"
            #endif
        };
        uint16_t x;   /* Full 16-bit X Index Register */
    };
    union {
        struct {
            #if defined(__LITTLE_ENDIAN__)
                uint8_t y_lo;  /* Lower 8 bits of Y Index Register */
                uint8_t y_hi;  /* Upper 8 bits of Y Index Register */
            #elif defined(__BIG_ENDIAN__)
                uint8_t y_hi;  /* Upper 8 bits of Y Index Register */
                uint8_t y_lo;  /* Lower 8 bits of Y Index Register */
            #else
                #error "Endianness not defined. Please define __LITTLE_ENDIAN__ or __BIG_ENDIAN__"
            #endif
        };
        uint16_t y;   /* Full 16-bit Y Index Register */
    };
    union {
        struct {
            uint8_t C : 1;  /* Carry Flag */
            uint8_t Z : 1;  /* Zero Flag */
            uint8_t I : 1;  /* Interrupt Disable Flag */
            uint8_t D : 1;  /* Decimal Mode Flag */
            uint8_t B : 1;  /* Break Command Flag */
            uint8_t _ : 1;  /* Unused bit */
            uint8_t V : 1;  /* Overflow Flag */
            uint8_t N : 1;  /* Negative Flag */
        };
        uint8_t p;  /* Processor Status Register */
    };
    uint64_t cycles; /* Number of cycles since reset */
    memory_map *memory;
    uint64_t last_tick;
    uint64_t cycle_duration_ns;
    uint64_t cycle_duration_ticks;    
    unsigned int free_run = 0;
};

extern struct cpu_state CPUs[MAX_CPUS];
