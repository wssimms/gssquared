#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <time.h>
#include <mach/mach_time.h>
#include <getopt.h>

#include "gs2.hpp"
#include "cpu.hpp"
#include "clock.hpp"
#include "memory.hpp"
#include "display/display.hpp"
#include "opcodes.hpp"
#include "debug.hpp"
#include "test.hpp"
#include "display/text_40x24.hpp"
#include "event_poll.hpp"
#include "devices/keyboard.hpp"
#include "devices/speaker.hpp"
#include "devices/loader.hpp"
#include "devices/thunderclockplus.hpp"
#include "platforms.hpp"

/**
 * References: 
 * Apple Machine Language: Don Inman, Kurt Inman
 * https://www.righto.com/2012/12/the-6502-overflow-flag-explained.html?m=1
 * https://www.masswerk.at/6502/6502_instruction_set.html#USBC
 * 
 */

/**
 * gssquared
 * 
 * Apple II/+/e/c/GS Emulator Extraordinaire
 * 
 * Main component Goals:
 *  - 6502 CPU (nmos) emulation
 *  - 65c02 CPU (cmos) emulation
 *  - 65sc816 CPU (8 and 16-bit) emulation
 *  - Display Emulation
 *     - Text - 40 / 80 col
 *     - Lores - 40 x 40, 40 x 48, 80 x 40, 80 x 48
 *     - Hires - 280x192 etc.
 *  - Disk I/O
 *   - 5.25 Emulation
 *   - 3.5 Emulation (SmartPort)
 *   - SCSI Card Emulation
 *  - Memory management emulate - a proposed MMU to allow multiple virtual CPUs to each have their own 16M address space
 * User should be able to select the Apple variant: 
 *  - Apple II
 *  - Apple II+
 *  - Apple IIe
 *  - Apple IIe Enhanced
 *  - Apple IIc
 *  - Apple IIc+
 *  - Apple IIGS
 * and edition of ROM.
 */

/**
 * TODO: Decide whether to implement the 'illegal' instructions. Maybe have that as a processor variant? the '816 does not implement them.
 * 
 */


/**
 * initialize memory
 */

void init_memory(cpu_state *cpu) {
    cpu->memory = new memory_map();
    
    #ifdef APPLEIIGS
    for (int i = 0; i < RAM_SIZE / GS2_PAGE_SIZE; i++) {
        cpu->memory->page_info[i].type = MEM_RAM;
        cpu->memory->pages[i] = new memory_page(); /* do we care if this is aligned */
        if (!cpu->memory->pages[i]) {
            std::cerr << "Failed to allocate memory page " << i << std::endl;
            exit(1);
        }
    }
    #else
    for (int i = 0; i < RAM_KB / GS2_PAGE_SIZE; i++) {
        cpu->memory->page_info[i].type = MEM_RAM;
        cpu->memory->pages[i] = new memory_page(); /* do we care if this is aligned */
        if (!cpu->memory->pages[i]) {
            std::cerr << "Failed to allocate memory page " << i << std::endl;
            exit(1);
        }
    }
    for (int i = 12; i <= 12; i++) {
        cpu->memory->page_info[i].type = MEM_IO;
        cpu->memory->pages[i] = new memory_page(); /* do we care if this is aligned */
        if (!cpu->memory->pages[i]) {
            std::cerr << "Failed to allocate memory page " << i << std::endl;
            exit(1);
        }
    }
    for (int i = 13; i <= 15; i++) {
        cpu->memory->page_info[i].type = MEM_ROM;
        cpu->memory->pages[i] = new memory_page(); /* do we care if this is aligned */
        if (!cpu->memory->pages[i]) {
            std::cerr << "Failed to allocate memory page " << i << std::endl;
            exit(1);
        }
    }
    #endif
}

uint64_t get_current_time_in_microseconds() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

void init_cpus() { // this is the same as a power-on event.
    for (int i = 0; i < MAX_CPUS; i++) {
        init_memory(&CPUs[i]);

        CPUs[i].boot_time = get_current_time_in_microseconds();
        CPUs[i].pc = 0x0400;
        CPUs[i].sp = rand() & 0xFF; // simulate a random stack pointer
        CPUs[i].a = 0;
        CPUs[i].x = 0;
        CPUs[i].y = 0;
        CPUs[i].p = 0;
        CPUs[i].cycles = 0;
        CPUs[i].last_tick = 0;

        set_clock_mode(&CPUs[i], CLOCK_1_024MHZ);

        CPUs[i].next_tick = mach_absolute_time() + CPUs[i].cycle_duration_ticks; 
    }
}

/**
 * This group of routines implement reused addressing mode calculations.
 *
 */

/**
 * Set the N and Z flags based on the value. Used by all arithmetic instructions, as well
 * as load instructions.
 */
inline void set_n_z_flags(cpu_state *cpu, uint8_t value) {
    cpu->Z = (value == 0);
    cpu->N = (value & 0x80) != 0;
}

inline void set_n_z_v_flags(cpu_state *cpu, uint8_t value, uint8_t N) {
    cpu->Z = (value == 0); // is A & M zero?
    cpu->N = (N & 0x80) != 0; // is M7 set?
    cpu->V = (N & 0x40) != 0; // is M6 set?
}

/**
 * Convert a binary-coded decimal byte to an integer.
 * Each nibble represents a decimal digit (0-9).
 */
inline uint8_t bcd_to_int(byte_t bcd) {
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

/**
 * Convert an integer (0-99) to binary-coded decimal format.
 * Returns a byte with each nibble representing a decimal digit.
 */
inline byte_t int_to_bcd(uint8_t n) {
    uint8_t n1 = n % 100;
    return ((n1 / 10) << 4) | (n1 % 10);
}


/**
 * perform accumulator addition. M is current value of accumulator. 
 * cpu: cpu flag
 * N: number being added to accumulator
 */
inline void add_and_set_flags(cpu_state *cpu, uint8_t N) {
    if (cpu->D == 0) {  // binary mode
        uint8_t M = cpu->a_lo;
        uint32_t S = M + N + (cpu->C);
        uint8_t S8 = (uint8_t) S;
        cpu->a_lo = S8;
        cpu->C = (S & 0x0100) >> 8;
        cpu->V =  !((M ^ N) & 0x80) && ((M ^ S8) & 0x80); // from 6502.org article https://www.righto.com/2012/12/the-6502-overflow-flag-explained.html?m=1
        set_n_z_flags(cpu, cpu->a_lo);
        if (DEBUG(DEBUG_REGISTERS)) fprintf(stdout, "   M: %02X  N: %02X  S: %02X  C: %02X  V: %02X", M, N, cpu->a_lo, cpu->C, cpu->V);
    } else {              // decimal mode
        uint8_t M = bcd_to_int(cpu->a_lo);
        uint8_t N1 = bcd_to_int(N);
        uint8_t S8 = M + N1 + cpu->C;
        cpu->a_lo = int_to_bcd(S8);
        cpu->C = (S8 > 99);
        set_n_z_flags(cpu, cpu->a_lo);
        if (DEBUG(DEBUG_REGISTERS)) fprintf(stdout, "   M: %02X  N: %02X  S: %02X  C: %02X  V: %02X", M, N, cpu->a_lo, cpu->C, cpu->V);
    }
}

// see https://www.righto.com/2012/12/the-6502-overflow-flag-explained.html?m=1
// "Subtraction on the 6502" section.

/* Does subtraction based on 3 inputs: M, N, and C. 
 * Sets flags, returns the result of the subtraction. But does not store it in the accumulator.
 * Caller may store the result in the accumulator if desired.
 */
inline uint8_t subtract_core(cpu_state *cpu, uint8_t M, uint8_t N, uint8_t C) {
    uint8_t N1 = N ^ 0xFF;
    uint32_t S = M + N1 + C;
    uint8_t S8 = (uint8_t) S;
    cpu->C = (S & 0x0100) >> 8;
    cpu->V =  !((M ^ N1) & 0x80) && ((M ^ S8) & 0x80);
    set_n_z_flags(cpu, S8);
    if (DEBUG(DEBUG_REGISTERS)) fprintf(stdout, "   M: %02X  N: %02X  S: %02X  C: %01X  V: %01X Z: %01X", M, N, S8, cpu->C, cpu->V, cpu->Z);
    return S8;
}

inline void subtract_and_set_flags(cpu_state *cpu, uint8_t N) {
    uint8_t C = cpu->C;

    if (cpu->D == 0) {
        uint8_t M = cpu->a_lo;

        uint8_t N1 = N ^ 0xFF;
        uint32_t S = M + N1 + C;
        uint8_t S8 = (uint8_t) S;
        cpu->C = (S & 0x0100) >> 8;
        cpu->V =  !((M ^ N1) & 0x80) && ((M ^ S8) & 0x80);
        cpu->a_lo = S8; // store the result in the accumulator. I accidentally deleted this before.
        set_n_z_flags(cpu, S8);
        if (DEBUG(DEBUG_REGISTERS)) fprintf(stdout, "   M: %02X  N: %02X  S: %02X  Z:%01X C:%01X N:%01X V:%01X ", M, N, S8, cpu->Z, cpu->C, cpu->N, cpu->V);
    } else {
        uint8_t M = bcd_to_int(cpu->a_lo);
        uint8_t N1 = bcd_to_int(N);
        int8_t S8 = M - N1 - !C;
        if (S8 < 0) {
            S8 += 100;
            cpu->C = 0; // c = 0 indicates a borrow in subtraction
        } else {
            cpu->C = 1;
        }
        cpu->a_lo = int_to_bcd(S8);
        set_n_z_flags(cpu, cpu->a_lo);
        if (DEBUG(DEBUG_REGISTERS)) fprintf(stdout, "   M: %02X  N: %02X  S: %02X  Z:%01X C:%01X N:%01X V:%01X ", M, N, S8, cpu->Z, cpu->C, cpu->N, cpu->V);
    }
}

/** nearly Identical algorithm to subtraction mostly.
 * Compares are the same as SBC with the carry flag assumed to be 1.
 * does not store the result in the accumulator
 * ALSO DOES NOT SET the V flag
 */
inline void compare_and_set_flags(cpu_state *cpu, uint8_t M, uint8_t N) {
    uint8_t C = 1;
    uint8_t N1 = N ^ 0xFF;
    uint32_t S = M + N1 + C;
    uint8_t S8 = (uint8_t) S;
    cpu->C = (S & 0x0100) >> 8;
    //cpu->V =  !((M ^ N1) & 0x80) && ((M ^ S8) & 0x80);
    set_n_z_flags(cpu, S8);
    //if (DEBUG) fprintf(stdout, "   M: %02X  N: %02X  S: %02X  C: %01X  V: %01X Z: %01X", M, N, S8, cpu->C, cpu->V, cpu->Z);
    if (DEBUG(DEBUG_REGISTERS)) fprintf(stdout, "   M: %02X  N: %02X  S: %02X  Z:%01X C:%01X N:%01X V:%01X ", M, N, S8, cpu->Z, cpu->C, cpu->N, cpu->V);

}

/**
 * Good discussion of branch instructions:
 * https://www.masswerk.at/6502/6502_instruction_set.html#BCC
 */
inline void branch_if(cpu_state *cpu, uint8_t N, bool condition) {
    uint16_t oaddr = cpu->pc;
    uint16_t taddr = oaddr + (int8_t) N;
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " => $%04X", taddr);

    if (condition) {
        if ((oaddr-2) == taddr) { // this test should back up 2 bytes.. so the infinite branch is actually bxx FE.
            fprintf(stdout, "JUMP TO SELF INFINITE LOOP Branch $%04X -> %01X\n", taddr, condition);
            cpu->halt = HLT_INSTRUCTION;
        }

        cpu->pc = cpu->pc + (int8_t) N;
        // branch taken uses another clock to update the PC
        incr_cycles(cpu); 
        /* If a branch is taken and the target is on a different page, this adds another CPU cycle (4 in total). */
        if ((oaddr & 0xFF00) != (taddr & 0xFF00)) {
            incr_cycles(cpu);
        }
        if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " (taken)");
    }
}


/**
 * Loading data for various addressing modes.
 * TODO: on ,X and ,Y we need to add cycle if a page boundary is crossed. (DONE)
 */

/** First, these methods (get_operand_address_*) read the type of operand from the PC,
 *  then read the operand from memory.
 * These are here because some routines read modify write, and we don't need to 
 * replicate that calculation.
 * */

/* there is no get_operand_address_immediate because functions below read the value itself from the PC */

/* These also generate the disassembly output - the operand address and mode */
inline zpaddr_t get_operand_address_zeropage(cpu_state *cpu) {
    zpaddr_t zpaddr = read_byte_from_pc(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " $%02X", zpaddr);
    return zpaddr;
}

inline zpaddr_t get_operand_address_zeropage_x(cpu_state *cpu) {
    zpaddr_t zpaddr = read_byte_from_pc(cpu);
    zpaddr_t taddr = zpaddr + cpu->x_lo; // make sure it wraps.
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " $%02X,X", zpaddr);
    return taddr;
}

inline zpaddr_t get_operand_address_zeropage_y(cpu_state *cpu) {
    zpaddr_t zpaddr = read_byte_from_pc(cpu);
    zpaddr_t taddr = zpaddr + cpu->y_lo; // make sure it wraps.
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " $%02X,Y", zpaddr);
    return taddr;
}

inline absaddr_t get_operand_address_absolute(cpu_state *cpu) {
    absaddr_t addr = read_word_from_pc(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " $%04X", addr);
    return addr;
}

inline absaddr_t get_operand_address_absolute_indirect(cpu_state *cpu) {
    absaddr_t addr = read_word_from_pc(cpu);
    absaddr_t taddr = read_word(cpu, addr);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " ($%04X) -> $%04X", addr, taddr);
    return taddr;
}

inline absaddr_t get_operand_address_absolute_x(cpu_state *cpu) {
    absaddr_t addr = read_word_from_pc(cpu);
    absaddr_t taddr = addr + cpu->x_lo;
    if ((addr & 0xFF00) != (taddr & 0xFF00)) {
        incr_cycles(cpu);
    }
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " $%04X,X", addr); // can add effective address here.
    return taddr;
}

inline absaddr_t get_operand_address_absolute_y(cpu_state *cpu) {
    absaddr_t addr = read_word_from_pc(cpu);
    absaddr_t taddr = addr + cpu->y_lo;
    if ((addr & 0xFF00) != (taddr & 0xFF00)) {
        incr_cycles(cpu);
    }
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " $%04X,Y", addr); // can add effective address here.
    return taddr;
}

inline uint16_t get_operand_address_indirect_x(cpu_state *cpu) {
    zpaddr_t zpaddr = read_byte_from_pc(cpu);
    absaddr_t taddr = read_word(cpu,(uint8_t)(zpaddr + cpu->x_lo)); // make sure it wraps.
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " ($%02X,X)  -> $%04X", zpaddr, taddr);
    return taddr;
}

inline absaddr_t get_operand_address_indirect_y(cpu_state *cpu) {
    zpaddr_t zpaddr = read_byte_from_pc(cpu);
    absaddr_t iaddr = read_word(cpu,zpaddr);
    absaddr_t taddr = iaddr + cpu->y_lo;

    if ((iaddr & 0xFF00) != (taddr & 0xFF00)) {
        incr_cycles(cpu);
    }
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " ($%02X),Y  -> $%04X", zpaddr, taddr);
    return taddr;
}

/** Second, these methods (get_operand_*) read the operand value from memory. */

inline byte_t get_operand_immediate(cpu_state *cpu) {
    byte_t N = read_byte_from_pc(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " #$%02X", N);
    return N;
}

inline byte_t get_operand_zeropage(cpu_state *cpu) {
    zpaddr_t addr = get_operand_address_zeropage(cpu);
    byte_t N = read_byte(cpu, addr);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " -> #$%02X", N);
    return N;
}

inline void store_operand_zeropage(cpu_state *cpu, byte_t N) {
    zpaddr_t addr = get_operand_address_zeropage(cpu);
    write_byte(cpu, addr, N);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "   [#%02X] -> $%02X", N, addr);
}

inline byte_t get_operand_zeropage_x(cpu_state *cpu) {
    zpaddr_t addr = get_operand_address_zeropage_x(cpu);
    byte_t N = read_byte(cpu, addr);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " [#%02X] <- $%02X", N, addr);
    return N;
}

inline void store_operand_zeropage_x(cpu_state *cpu, byte_t N) {
    zpaddr_t addr = get_operand_address_zeropage_x(cpu);
    write_byte(cpu, addr, N);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "  [#%02X] -> $%02X", N, addr);
}

inline byte_t get_operand_zeropage_y(cpu_state *cpu) {
    zpaddr_t zpaddr = get_operand_address_zeropage_y(cpu);
    byte_t N = read_byte(cpu, zpaddr);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "   [#%02X] <- $%02X", N, zpaddr);
    return N;
}

inline void store_operand_zeropage_y(cpu_state *cpu, byte_t N) {
    zpaddr_t zpaddr = get_operand_address_zeropage_y(cpu);
    write_byte(cpu, zpaddr, N);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "  [#%02X] -> $%02X", N, zpaddr);
}

inline byte_t get_operand_zeropage_indirect_x(cpu_state *cpu) {
    absaddr_t taddr = get_operand_address_indirect_x(cpu);
    byte_t N = read_byte(cpu, taddr);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "  [#%02X] <- $%04X", N, taddr);
    return N;
}

inline void store_operand_zeropage_indirect_x(cpu_state *cpu, byte_t N) {
    absaddr_t taddr = get_operand_address_indirect_x(cpu);
    write_byte(cpu, taddr, N);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "   [#%02X] <- $%04X", N, taddr);
}

inline byte_t get_operand_zeropage_indirect_y(cpu_state *cpu) {
    absaddr_t addr = get_operand_address_indirect_y(cpu);
    byte_t N = read_byte(cpu, addr);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "  [#%02X] <- $%04X", N, addr);
    return N;
}

inline void store_operand_zeropage_indirect_y(cpu_state *cpu, byte_t N) {
    absaddr_t addr = get_operand_address_indirect_y(cpu);
    write_byte(cpu, addr, N);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "   [#%02X] -> $%04X", N, addr);
}

inline byte_t get_operand_absolute(cpu_state *cpu) {
    absaddr_t addr = get_operand_address_absolute(cpu);
    byte_t N = read_byte(cpu, addr);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "   [#%02X] <- $%04X", N, addr);
    return N;
}

inline void store_operand_absolute(cpu_state *cpu, byte_t N) {
    absaddr_t addr = get_operand_address_absolute(cpu);
    write_byte(cpu, addr, N);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "   [#%02X] <- $%04X", N, addr);
}

inline byte_t get_operand_absolute_x(cpu_state *cpu) {
    absaddr_t addr = get_operand_address_absolute_x(cpu);
    byte_t N = read_byte(cpu, addr);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "   [#%02X] <- $%04X", N, addr);
    return N;
}

inline void store_operand_absolute_x(cpu_state *cpu, byte_t N) {
    absaddr_t addr = get_operand_address_absolute_x(cpu);
    write_byte(cpu, addr, N);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "   [#%02X] -> $%04X", N, addr);
}

inline byte_t get_operand_absolute_y(cpu_state *cpu) {
    absaddr_t addr = get_operand_address_absolute_y(cpu);
    byte_t N = read_byte(cpu, addr);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "   [#%02X] <- $%04X", N, addr);
    return N;
}

inline void store_operand_absolute_y(cpu_state *cpu, byte_t N) {
    absaddr_t addr = get_operand_address_absolute_y(cpu);
    write_byte(cpu, addr, N);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "   [#%02X] -> $%04X", N, addr);
}

inline byte_t get_operand_relative(cpu_state *cpu) {
    byte_t N = read_byte_from_pc(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " #$%02X", N);
    return N;
}


inline void op_transfer_to_x(cpu_state *cpu, byte_t N) {
    cpu->x_lo = N;
    set_n_z_flags(cpu, cpu->x_lo);
    incr_cycles(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "[#$%02X] -> X", N);
}

inline void op_transfer_to_y(cpu_state *cpu, byte_t N) {
    cpu->y_lo = N;
    set_n_z_flags(cpu, cpu->y_lo);
    incr_cycles(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "[#$%02X] -> Y", N);
}

inline void op_transfer_to_a(cpu_state *cpu, byte_t N) {
    cpu->a_lo = N;
    set_n_z_flags(cpu, cpu->a_lo);
    incr_cycles(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "[#$%02X] -> A", N);
}

inline void op_transfer_to_s(cpu_state *cpu, byte_t N) {
    cpu->sp = N;
    incr_cycles(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "[#$%02X] -> S", N);
}

/**
 * Decrement an operand.
 */
inline void dec_operand(cpu_state *cpu, absaddr_t addr) {
    byte_t N = read_byte(cpu, addr);
    N--;
    incr_cycles(cpu);
    write_byte(cpu, addr, N);
    set_n_z_flags(cpu, N);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "   [#%02X]", N);
}

inline void dec_operand_zeropage(cpu_state *cpu) {
    zpaddr_t zpaddr = get_operand_address_zeropage(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " $%02X", zpaddr);
    dec_operand(cpu, zpaddr);
}

inline void dec_operand_zeropage_x(cpu_state *cpu) {
    zpaddr_t zpaddr = get_operand_address_zeropage_x(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " $%02X", zpaddr);
    dec_operand(cpu, zpaddr);
}

inline void dec_operand_absolute(cpu_state *cpu) {
    absaddr_t addr = get_operand_address_absolute(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " $%04X", addr);
    dec_operand(cpu, addr);
}

inline void dec_operand_absolute_x(cpu_state *cpu) {
    absaddr_t addr = get_operand_address_absolute_x(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " $%04X", addr);
    dec_operand(cpu, addr);
}

/**
 * Increment an operand.
 */
inline void inc_operand(cpu_state *cpu, absaddr_t addr) {
    byte_t N = read_byte(cpu, addr);
    N++;
    incr_cycles(cpu);
    write_byte(cpu, addr, N);
    set_n_z_flags(cpu, N);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "   [#%02X]", N);
}

inline void inc_operand_zeropage(cpu_state *cpu) {
    zpaddr_t zpaddr = get_operand_address_zeropage(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " $%02X", zpaddr);
    inc_operand(cpu, zpaddr);
}

inline void inc_operand_zeropage_x(cpu_state *cpu) {
    zpaddr_t zpaddr = get_operand_address_zeropage_x(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " $%02X", zpaddr);
    inc_operand(cpu, zpaddr);
}

inline void inc_operand_absolute(cpu_state *cpu) {
    absaddr_t addr = get_operand_address_absolute(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " $%04X", addr);
    inc_operand(cpu, addr);
}

inline void inc_operand_absolute_x(cpu_state *cpu) {
    absaddr_t addr = get_operand_address_absolute_x(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " $%04X", addr);
    inc_operand(cpu, addr);
}

inline byte_t logical_shift_right(cpu_state *cpu, byte_t N) {
    uint8_t C = N & 0x01;
    N = N >> 1;
    cpu->C = C;
    set_n_z_flags(cpu, N);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " [#%02X]", N);
    return N;
}

inline byte_t logical_shift_right_addr(cpu_state *cpu, absaddr_t addr) {
    byte_t N = read_byte(cpu, addr);
    byte_t result = logical_shift_right(cpu, N);
    write_byte(cpu, addr, result);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " -> $%04X", addr);
    return result;
}

inline byte_t arithmetic_shift_left(cpu_state *cpu, byte_t N) {
    uint8_t C = (N & 0x80) >> 7;
    N = N << 1;
    cpu->C = C;
    set_n_z_flags(cpu, N);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " [#%02X]", N);
    return N;
}

inline byte_t arithmetic_shift_left_addr(cpu_state *cpu, absaddr_t addr) {
    byte_t N = read_byte(cpu, addr);
    byte_t result = arithmetic_shift_left(cpu, N);
    write_byte(cpu, addr, result);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " -> $%04X", addr);
    return result;
}


inline byte_t rotate_right(cpu_state *cpu, byte_t N) {
    uint8_t C = N & 0x01;
    N = N >> 1;
    N |= (cpu->C << 7);
    cpu->C = C;
    set_n_z_flags(cpu, N);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " [#%02X]", N);
    return N;
}

inline byte_t rotate_right_addr(cpu_state *cpu, absaddr_t addr) {
    byte_t N = read_byte(cpu, addr);
    byte_t result = rotate_right(cpu, N);
    write_byte(cpu, addr, result);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " -> $%04X", addr);
    return result;
}

inline byte_t rotate_left(cpu_state *cpu, byte_t N) {
    uint8_t C = ((N & 0x80) != 0);
    N = N << 1;
    N |= cpu->C;
    cpu->C = C;
    set_n_z_flags(cpu, N);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " [#%02X]", N);
    return N;
}

inline byte_t rotate_left_addr(cpu_state *cpu, absaddr_t addr) {
    byte_t N = read_byte(cpu, addr);
    byte_t result = rotate_left(cpu, N);
    write_byte(cpu, addr, result);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " -> $%04X", addr);
    return result;
}

inline void push_byte(cpu_state *cpu, byte_t N) {
    write_byte(cpu, 0x0100 + cpu->sp, N);
    cpu->sp = (uint8_t)(cpu->sp - 1);
    incr_cycles(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " [#%02X] -> S[0x01 %02X]", N, cpu->sp + 1);
}

inline byte_t pop_byte(cpu_state *cpu) {
    cpu->sp = (uint8_t)(cpu->sp + 1);
    byte_t N = read_byte(cpu, 0x0100 + cpu->sp);
    incr_cycles(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " [#%02X] <- S[0x01 %02X]", N, cpu->sp - 1);
    return N;
}

inline void push_word(cpu_state *cpu, word_t N) {
    write_byte(cpu, 0x0100 + cpu->sp, (N & 0xFF00) >> 8);
    write_byte(cpu, 0x0100 + cpu->sp - 1, N & 0x00FF);
    cpu->sp = (uint8_t)(cpu->sp - 2);
    incr_cycles(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " [#%04X] -> S[0x01 %02X]", N, cpu->sp + 1);
}

inline absaddr_t pop_word(cpu_state *cpu) {
    absaddr_t N = read_word(cpu, 0x0100 + cpu->sp + 1);
    cpu->sp = (uint8_t)(cpu->sp + 2);
    incr_cycles(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, " [#%04X] <- S[0x01 %02X]", N, cpu->sp - 1);
    return N;
}

int execute_next_6502(cpu_state *cpu) {

    if (DEBUG(DEBUG_CLOCK)) {
        uint64_t current_time = get_current_time_in_microseconds();
        fprintf(stdout, "[ %llu ]", cpu->cycles);
        uint64_t elapsed_time = current_time - cpu->boot_time;
        fprintf(stdout, "[eTime: %llu] ", elapsed_time);
        float_t cycles_per_second = (cpu->cycles * 1000000000.0) / (elapsed_time * 1000.0);
        fprintf(stdout, "[eHz: %.0f] ", cycles_per_second);
    }

    if ( DEBUG(DEBUG_TIMEDRUN) && cpu->cycles > (10 * cpu->HZ_RATE) ) { // should take about 10 seconds.
        uint64_t current_time = get_current_time_in_microseconds();
        uint64_t elapsed_time = current_time - cpu->boot_time;
        fprintf(stdout, "[eTime: %llu] ", elapsed_time);
        float_t cycles_per_second = (cpu->cycles * 1000000000.0) / (elapsed_time * 1000.0);
        fprintf(stdout, "[eHz: %f] ", cycles_per_second);
        cpu->halt = HLT_INSTRUCTION;
    }

    if (DEBUG(DEBUG_REGISTERS)) fprintf(stdout, " | PC: $%04X, A: $%02X, X: $%02X, Y: $%02X, P: $%02X, S: $%02X || ", cpu->pc, cpu->a_lo, cpu->x_lo, cpu->y_lo, cpu->p, cpu->sp);

    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "%04X: ", cpu->pc); // so PC is correct.
    opcode_t opcode = read_byte_from_pc(cpu);
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "%s", get_opcode_name(opcode));

    switch (opcode) {

        /* ADC --------------------------------- */
        case OP_ADC_IMM: /* ADC Immediate */
            {
                byte_t N = get_operand_immediate(cpu);
                add_and_set_flags(cpu, N);                    
            }
            break;

        case OP_ADC_ZP: /* ADC ZP */
            {
                byte_t N = get_operand_zeropage(cpu);
                add_and_set_flags(cpu, N);
            }
            break;

        case OP_ADC_ZP_X: /* ADC ZP, X */
            {
                byte_t N = get_operand_zeropage_x(cpu);
                add_and_set_flags(cpu, N);
            }
            break;

        case OP_ADC_ABS: /* ADC Absolute */
            {
                byte_t N = get_operand_absolute(cpu);
                add_and_set_flags(cpu, N);
            }
            break;

        case OP_ADC_ABS_X: /* ADC Absolute, X */
            {
                byte_t N = get_operand_absolute_x(cpu);
                add_and_set_flags(cpu, N);
            }
            break;

        case OP_ADC_ABS_Y: /* ADC Absolute, Y */
            {
                byte_t N = get_operand_absolute_y(cpu);
                add_and_set_flags(cpu, N);
            }
            break;

        case OP_ADC_IND_X: /* ADC (Indirect, X) */
            {
                byte_t N = get_operand_zeropage_indirect_x(cpu);
                add_and_set_flags(cpu, N);
            }
            break;

        case OP_ADC_IND_Y: /* ADC (Indirect), Y */
            {
                byte_t N = get_operand_zeropage_indirect_y(cpu);
                add_and_set_flags(cpu, N);
            }
            break;

    /* AND --------------------------------- */

        case OP_AND_IMM: /* AND Immediate */
            {
                byte_t N = get_operand_immediate(cpu);
                cpu->a_lo &= N; // replace with an and_and_set_flags 
                set_n_z_flags(cpu, cpu->a_lo); 
            }
            break;

        case OP_AND_ZP: /* AND Zero Page */
            {
                byte_t N = get_operand_zeropage(cpu);
                cpu->a_lo &= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_AND_ZP_X: /* AND Zero Page, X */
            {
                byte_t N = get_operand_zeropage_x(cpu);
                cpu->a_lo &= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_AND_ABS: /* AND Absolute */
            {
                byte_t N = get_operand_absolute(cpu);
                cpu->a_lo &= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;
        
        case OP_AND_ABS_X: /* AND Absolute, X */
            {
                byte_t N = get_operand_absolute_x(cpu);
                cpu->a_lo &= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_AND_ABS_Y: /* AND Absolute, Y */
            {
                byte_t N = get_operand_absolute_y(cpu);
                cpu->a_lo &= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_AND_IND_X: /* AND (Indirect, X) */
            {
                byte_t N = get_operand_zeropage_indirect_x(cpu);
                cpu->a_lo &= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_AND_IND_Y: /* AND (Indirect), Y */
            {
                byte_t N = get_operand_zeropage_indirect_y(cpu);
                cpu->a_lo &= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

    /* ASL --------------------------------- */

    case OP_ASL_ACC: /* ASL Accumulator */
        {
            byte_t N = cpu->a_lo;
            cpu->a_lo = arithmetic_shift_left(cpu, N);
        }
        break;

    case OP_ASL_ZP: /* ASL Zero Page */
        {
            absaddr_t addr = get_operand_address_zeropage(cpu);
            arithmetic_shift_left_addr(cpu, addr);
        }
        break;

    case OP_ASL_ZP_X: /* ASL Zero Page, X */
        {
            zpaddr_t zpaddr = get_operand_address_zeropage_x(cpu);
            arithmetic_shift_left_addr(cpu, zpaddr);
        }
        break;

    case OP_ASL_ABS: /* ASL Absolute */
        {
            absaddr_t addr = get_operand_address_absolute(cpu);
            arithmetic_shift_left_addr(cpu, addr);
        }
        break;
        
    case OP_ASL_ABS_X: /* ASL Absolute, X */
        {
            absaddr_t addr = get_operand_address_absolute_x(cpu);
            arithmetic_shift_left_addr(cpu, addr);
        }
        break;

    /* Branching --------------------------------- */
        case OP_BCC_REL: /* BCC Relative */
            {
                byte_t N = get_operand_relative(cpu);
                branch_if(cpu, N, cpu->C == 0);
            }
            break;

        case OP_BCS_REL: /* BCS Relative */
            {
                byte_t N = get_operand_relative(cpu);
                branch_if(cpu, N, cpu->C == 1);
            }
            break;

        case OP_BEQ_REL: /* BEQ Relative */
            {
                byte_t N = get_operand_relative(cpu);
                branch_if(cpu, N, cpu->Z == 1);
            }
            break;

        case OP_BNE_REL: /* BNE Relative */
            {
                byte_t N = get_operand_relative(cpu);
                branch_if(cpu, N, cpu->Z == 0);
            }
            break;

        case OP_BMI_REL: /* BMI Relative */
            {
                byte_t N = get_operand_relative(cpu);
                branch_if(cpu, N, cpu->N == 1);
            }
            break;

        case OP_BPL_REL: /* BPL Relative */
            {
                byte_t N = get_operand_relative(cpu);
                branch_if(cpu, N, cpu->N == 0);
            }
            break;

        case OP_BVC_REL: /* BVC Relative */
            {
                uint8_t N = get_operand_relative(cpu);
                branch_if(cpu, N, cpu->V == 0);
            }
            break;

        case OP_BVS_REL: /* BVS Relative */
            {
                byte_t N = get_operand_relative(cpu);
                branch_if(cpu, N, cpu->V == 1);
            }
            break;

    /* CMP --------------------------------- */
        case OP_CMP_IMM: /* CMP Immediate */
            {
                byte_t N = get_operand_immediate(cpu);
                compare_and_set_flags(cpu, cpu->a_lo, N);
            }
            break;

        case OP_CMP_ZP: /* CMP Zero Page */
            {
                byte_t N = get_operand_zeropage(cpu);
                compare_and_set_flags(cpu, cpu->a_lo, N);
            }
            break;

        case OP_CMP_ZP_X: /* CMP Zero Page, X */
            {
                byte_t N = get_operand_zeropage_x(cpu);
                compare_and_set_flags(cpu, cpu->a_lo, N);
            }
            break;

        case OP_CMP_ABS: /* CMP Absolute */
            {
                byte_t N = get_operand_absolute(cpu);
                compare_and_set_flags(cpu, cpu->a_lo, N);
            }
            break;

        case OP_CMP_ABS_X: /* CMP Absolute, X */
            {
                byte_t N = get_operand_absolute_x(cpu);
                compare_and_set_flags(cpu, cpu->a_lo, N);
            }
            break;

        case OP_CMP_ABS_Y: /* CMP Absolute, Y */
            {
                byte_t N = get_operand_absolute_y(cpu);
                compare_and_set_flags(cpu, cpu->a_lo, N);
            }
            break;

        case OP_CMP_IND_X: /* CMP (Indirect, X) */
            {
                byte_t N = get_operand_zeropage_indirect_x(cpu);
                compare_and_set_flags(cpu, cpu->a_lo, N);
            }
            break;

        case OP_CMP_IND_Y: /* CMP (Indirect), Y */
            {
                byte_t N = get_operand_zeropage_indirect_y(cpu);
                compare_and_set_flags(cpu, cpu->a_lo, N);
            }
            break;

    /* CPX --------------------------------- */
        case OP_CPX_IMM: /* CPX Immediate */
            {
                byte_t N = get_operand_immediate(cpu);
                compare_and_set_flags(cpu, cpu->x_lo, N);
            }
            break;

        case OP_CPX_ZP: /* CPX Zero Page */
            {
                byte_t N = get_operand_zeropage(cpu);
                compare_and_set_flags(cpu, cpu->x_lo, N);
            }
            break;

        case OP_CPX_ABS: /* CPX Absolute */
            {
                byte_t N = get_operand_absolute(cpu);
                compare_and_set_flags(cpu, cpu->x_lo, N);
            }
            break;

    /* CPY --------------------------------- */
        case OP_CPY_IMM: /* CPY Immediate */
            {
                byte_t N = get_operand_immediate(cpu);
                compare_and_set_flags(cpu, cpu->y_lo, N);
            }
            break;

        case OP_CPY_ZP: /* CPY Zero Page */
            {
                byte_t N = get_operand_zeropage(cpu);
                compare_and_set_flags(cpu, cpu->y_lo, N);
            }
            break;

        case OP_CPY_ABS: /* CPY Absolute */
            {
                byte_t N = get_operand_absolute(cpu);
                compare_and_set_flags(cpu, cpu->y_lo, N);
            }
            break;

    /* DEC --------------------------------- */
        case OP_DEC_ZP: /* DEC Zero Page */
            {
                dec_operand_zeropage(cpu);
            }
            break;

        case OP_DEC_ZP_X: /* DEC Zero Page, X */
            {
                dec_operand_zeropage_x(cpu);
            }
            break;

        case OP_DEC_ABS: /* DEC Absolute */
            {
                dec_operand_absolute(cpu);
            }
            break;

        case OP_DEC_ABS_X: /* DEC Absolute, X */
            {
                dec_operand_absolute_x(cpu);
            }
            break;

    /* DE(xy) --------------------------------- */
        case OP_DEX_IMP: /* DEX Implied */
            {
                cpu->x_lo --;
                incr_cycles(cpu);
                set_n_z_flags(cpu, cpu->x_lo);
            }
            break;

        case OP_DEY_IMP: /* DEY Implied */
            {
                cpu->y_lo --;
                incr_cycles(cpu);
                set_n_z_flags(cpu, cpu->y_lo);
            }
            break;


    /* EOR --------------------------------- */

        case OP_EOR_IMM: /* EOR Immediate */
            {
                byte_t N = get_operand_immediate(cpu);
                cpu->a_lo ^= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_EOR_ZP: /* EOR Zero Page */
            {
                byte_t N = get_operand_zeropage(cpu);
                cpu->a_lo ^= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_EOR_ZP_X: /* EOR Zero Page, X */
            {
                byte_t N = get_operand_zeropage_x(cpu);
                cpu->a_lo ^= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_EOR_ABS: /* EOR Absolute */
            {
                byte_t N = get_operand_absolute(cpu);
                cpu->a_lo ^= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;
        
        case OP_EOR_ABS_X: /* EOR Absolute, X */
            {
                byte_t N = get_operand_absolute_x(cpu);
                cpu->a_lo ^= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_EOR_ABS_Y: /* EOR Absolute, Y */
            {
                byte_t N = get_operand_absolute_y(cpu);
                cpu->a_lo ^= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_EOR_IND_X: /* EOR (Indirect, X) */
            {
                byte_t N = get_operand_zeropage_indirect_x(cpu);
                cpu->a_lo ^= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_EOR_IND_Y: /* EOR (Indirect), Y */
            {
                byte_t N = get_operand_zeropage_indirect_y(cpu);
                cpu->a_lo ^= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

    /* INC --------------------------------- */
        case OP_INC_ZP: /* INC Zero Page */
            {
                inc_operand_zeropage(cpu);
            }
            break;

        case OP_INC_ZP_X: /* INC Zero Page, X */
            {
                inc_operand_zeropage_x(cpu);
            }
            break;

        case OP_INC_ABS: /* INC Absolute */
            {
                inc_operand_absolute(cpu);
            }
            break;

        case OP_INC_ABS_X: /* INC Absolute, X */
            {
                inc_operand_absolute_x(cpu);
            }
            break;

    /* IN(xy) --------------------------------- */

        case OP_INX_IMP: /* INX Implied */
            {
                cpu->x_lo ++;
                incr_cycles(cpu);
                set_n_z_flags(cpu, cpu->x_lo);
            }
            break;

        case OP_INY_IMP: /* INY Implied */
            {
                cpu->y_lo ++;
                incr_cycles(cpu);
                set_n_z_flags(cpu, cpu->y_lo);
            }
            break;

    /* LDA --------------------------------- */

        case OP_LDA_IMM: /* LDA Immediate */
            {
                cpu->a_lo = get_operand_immediate(cpu);
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_LDA_ZP: /* LDA Zero Page */
            {
                cpu->a_lo =  get_operand_zeropage(cpu);
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_LDA_ZP_X: /* LDA Zero Page, X */
            {
                cpu->a_lo = get_operand_zeropage_x(cpu);
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_LDA_ABS: /* LDA Absolute */
            {
                cpu->a_lo = get_operand_absolute(cpu);
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;
        
        case OP_LDA_ABS_X: /* LDA Absolute, X */
            {
                cpu->a_lo = get_operand_absolute_x(cpu);
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_LDA_ABS_Y: /* LDA Absolute, Y */
            {
                cpu->a_lo = get_operand_absolute_y(cpu);
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_LDA_IND_X: /* LDA (Indirect, X) */
            {
                cpu->a_lo = get_operand_zeropage_indirect_x(cpu);
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_LDA_IND_Y: /* LDA (Indirect), Y */
            {
                cpu->a_lo = get_operand_zeropage_indirect_y(cpu);
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        /* LDX --------------------------------- */

        case OP_LDX_IMM: /* LDX Immediate */
            {
                cpu->x_lo = get_operand_immediate(cpu);
                set_n_z_flags(cpu, cpu->x_lo);
            }
            break;

        case OP_LDX_ZP: /* LDX Zero Page */
            {
                cpu->x_lo = get_operand_zeropage(cpu);
                set_n_z_flags(cpu, cpu->x_lo);
            }
            break;

        case OP_LDX_ZP_Y: /* LDX Zero Page, Y */
            {
                cpu->x_lo = get_operand_zeropage_y(cpu);
                set_n_z_flags(cpu, cpu->x_lo);
            }
            break;

        case OP_LDX_ABS: /* LDX Absolute */
            {
                cpu->x_lo = get_operand_absolute(cpu);
                set_n_z_flags(cpu, cpu->x_lo);
            }
            break;

        case OP_LDX_ABS_Y: /* LDX Absolute, Y */
            {
                cpu->x_lo = get_operand_absolute_y(cpu);
                set_n_z_flags(cpu, cpu->x_lo);
            }
            break;

        /* LDY --------------------------------- */

        case OP_LDY_IMM: /* LDY Immediate */
            {
                byte_t N = get_operand_immediate(cpu);
                cpu->y_lo = N;
                set_n_z_flags(cpu, cpu->y_lo);
            }
            break;
        
        case OP_LDY_ZP: /* LDY Zero Page */
            {
                byte_t N = get_operand_zeropage(cpu);
                cpu->y_lo = N;
                set_n_z_flags(cpu, cpu->y_lo);
            }
            break;

        case OP_LDY_ZP_X: /* LDY Zero Page, X */
            {
                byte_t N = get_operand_zeropage_x(cpu);
                cpu->y_lo = N;
                set_n_z_flags(cpu, cpu->y_lo);
            }
            break;

        case OP_LDY_ABS: /* LDY Absolute */
            {
                byte_t N = get_operand_absolute(cpu);
                cpu->y_lo = N;
                set_n_z_flags(cpu, cpu->y_lo);
            }
            break;

        case OP_LDY_ABS_X: /* LDY Absolute, X */
            {
                byte_t N = get_operand_absolute_x(cpu);
                cpu->y_lo = N;
                set_n_z_flags(cpu, cpu->y_lo);
            }
            break;

    /* LSR  --------------------------------- */

        case OP_LSR_ACC: /* LSR Accumulator */
            {
                byte_t N = cpu->a_lo;
                cpu->a_lo = logical_shift_right(cpu, N);
            }
            break;

        case OP_LSR_ZP: /* LSR Zero Page */
            {
                absaddr_t addr = get_operand_address_zeropage(cpu);
                logical_shift_right_addr(cpu, addr);
            }
            break;

        case OP_LSR_ZP_X: /* LSR Zero Page, X */
            {
                absaddr_t addr = get_operand_address_zeropage_x(cpu);
                logical_shift_right_addr(cpu, addr);
            }
            break;

        case OP_LSR_ABS: /* LSR Absolute */
            {
                absaddr_t addr = get_operand_address_absolute(cpu);
                logical_shift_right_addr(cpu, addr);
            }
            break;

        case OP_LSR_ABS_X: /* LSR Absolute, X */
            {
                absaddr_t addr = get_operand_address_absolute_x(cpu);
                logical_shift_right_addr(cpu, addr);
            }
            break;


    /* ORA --------------------------------- */

        case OP_ORA_IMM: /* AND Immediate */
            {
                byte_t N = get_operand_immediate(cpu);
                cpu->a_lo |= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_ORA_ZP: /* AND Zero Page */
            {
                byte_t N = get_operand_zeropage(cpu);
                cpu->a_lo |= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_ORA_ZP_X: /* AND Zero Page, X */
            {
                byte_t N = get_operand_zeropage_x(cpu);
                cpu->a_lo |= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_ORA_ABS: /* AND Absolute */
            {
                byte_t N = get_operand_absolute(cpu);
                cpu->a_lo |= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;
        
        case OP_ORA_ABS_X: /* AND Absolute, X */
            {
                byte_t N = get_operand_absolute_x(cpu);
                cpu->a_lo |= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_ORA_ABS_Y: /* AND Absolute, Y */
            {
                byte_t N = get_operand_absolute_y(cpu);
                cpu->a_lo |= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_ORA_IND_X: /* AND (Indirect, X) */
            {
                byte_t N = get_operand_zeropage_indirect_x(cpu);
                cpu->a_lo |= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

        case OP_ORA_IND_Y: /* AND (Indirect), Y */
            {
                byte_t N = get_operand_zeropage_indirect_y(cpu);
                cpu->a_lo |= N;
                set_n_z_flags(cpu, cpu->a_lo);
            }
            break;

    /* Stack operations --------------------------------- */

        case OP_PHA_IMP: /* PHA Implied */
            {
                push_byte(cpu, cpu->a_lo);
            }
            break;

        case OP_PHP_IMP: /* PHP Implied */
            {
                push_byte(cpu, (cpu->p | (FLAG_B | FLAG_UNUSED))); // break flag and Unused bit set to 1.
            }
            break;

        case OP_PLP_IMP: /* PLP Implied */
            {
                cpu->p = pop_byte(cpu) & ~FLAG_B; // break flag is cleared.
            }
            break;

        case OP_PLA_IMP: /* PLA Implied */
            {
                cpu->a_lo = pop_byte(cpu);
                set_n_z_flags(cpu, cpu->a_lo);
                incr_cycles(cpu);
            }
            break;
    /* ROL --------------------------------- */

        case OP_ROL_ACC: /* ROL Accumulator */
            {
                byte_t N = cpu->a_lo;
                cpu->a_lo = rotate_left(cpu, N);
            }
            break;

        case OP_ROL_ZP: /* ROL Zero Page */
            {
                absaddr_t addr = get_operand_address_zeropage(cpu);
                rotate_left_addr(cpu, addr);
            }
            break;

        case OP_ROL_ZP_X: /* ROL Zero Page, X */
            {
                absaddr_t addr = get_operand_address_zeropage_x(cpu);
                rotate_left_addr(cpu, addr);
            }
            break;

        case OP_ROL_ABS: /* ROL Absolute */
            {
                absaddr_t addr = get_operand_address_absolute(cpu);
                rotate_left_addr(cpu, addr);
            }
            break;

        case OP_ROL_ABS_X: /* ROL Absolute, X */
            {
                absaddr_t addr = get_operand_address_absolute_x(cpu);
                rotate_left_addr(cpu, addr);
            }
            break;

    /* ROR --------------------------------- */
        case OP_ROR_ACC: /* ROR Accumulator */
            {
                byte_t N = cpu->a_lo;
                cpu->a_lo = rotate_right(cpu, N);
            }
            break;

        case OP_ROR_ZP: /* ROR Zero Page */
            {
                absaddr_t addr = get_operand_address_zeropage(cpu);
                rotate_right_addr(cpu, addr);
            }
            break;

        case OP_ROR_ZP_X: /* ROR Zero Page, X */
            {
                absaddr_t addr = get_operand_address_zeropage_x(cpu);
                rotate_right_addr(cpu, addr);
            }
            break;

        case OP_ROR_ABS: /* ROR Absolute */
            {
                absaddr_t addr = get_operand_address_absolute(cpu);
                rotate_right_addr(cpu, addr);
            }
            break;

        case OP_ROR_ABS_X: /* ROR Absolute, X */
            {
                absaddr_t addr = get_operand_address_absolute_x(cpu);
                rotate_right_addr(cpu, addr);
            }
            break;

        /* SBC --------------------------------- */
        case OP_SBC_IMM: /* SBC Immediate */
            {
                byte_t N = get_operand_immediate(cpu);
                subtract_and_set_flags(cpu, N);
            }
            break;

        case OP_SBC_ZP: /* SBC Zero Page */
            {
                byte_t N = get_operand_zeropage(cpu);
                subtract_and_set_flags(cpu, N);
            }
            break;

        case OP_SBC_ZP_X: /* SBC Zero Page, X */
            {
                byte_t N = get_operand_zeropage_x(cpu);
                subtract_and_set_flags(cpu, N);
            }
            break;

        case OP_SBC_ABS: /* SBC Absolute */
            {
                byte_t N = get_operand_absolute(cpu);
                subtract_and_set_flags(cpu, N);
            }
            break;

        case OP_SBC_ABS_X: /* SBC Absolute, X */
            {
                byte_t N = get_operand_absolute_x(cpu);
                subtract_and_set_flags(cpu, N);
            }
            break;

        case OP_SBC_ABS_Y: /* SBC Absolute, Y */
            {
                byte_t N = get_operand_absolute_y(cpu);
                subtract_and_set_flags(cpu, N);
            }
            break;

        case OP_SBC_IND_X: /* SBC (Indirect, X) */
            {
                byte_t N = get_operand_zeropage_indirect_x(cpu);
                subtract_and_set_flags(cpu, N);
            }
            break;

        case OP_SBC_IND_Y: /* SBC (Indirect), Y */
            {
                byte_t N = get_operand_zeropage_indirect_y(cpu);
                subtract_and_set_flags(cpu, N);
            }
            break;

        /* STA --------------------------------- */
        case OP_STA_ZP: /* STA Zero Page */
            {
                store_operand_zeropage(cpu, cpu->a_lo);
            }
            break;

        case OP_STA_ZP_X: /* STA Zero Page, X */
            {
                store_operand_zeropage_x(cpu, cpu->a_lo);
            }
            break;

        case OP_STA_ABS: /* STA Absolute */
            {
                store_operand_absolute(cpu, cpu->a_lo);
            }
            break;

        case OP_STA_ABS_X: /* STA Absolute, X */
            {
                store_operand_absolute_x(cpu, cpu->a_lo);
            }
            break;

        case OP_STA_ABS_Y: /* STA Absolute, Y */
            {
                store_operand_absolute_y(cpu, cpu->a_lo);
            }
            break;

        case OP_STA_IND_X: /* STA (Indirect, X) */
            {
                store_operand_zeropage_indirect_x(cpu, cpu->a_lo);
            }
            break;

        case OP_STA_IND_Y: /* STA (Indirect), Y */
            {
                store_operand_zeropage_indirect_y(cpu, cpu->a_lo);
            }
            break;
        
        /* STX --------------------------------- */
        case OP_STX_ZP: /* STX Zero Page */
            {
                store_operand_zeropage(cpu, cpu->x_lo);
            }
            break;

        case OP_STX_ZP_Y: /* STX Zero Page, Y */
            {
                store_operand_zeropage_y(cpu, cpu->x_lo);
            }
            break;

        case OP_STX_ABS: /* STX Absolute */
            {
                store_operand_absolute(cpu, cpu->x_lo);
            }
            break;

    /* STY --------------------------------- */
        case OP_STY_ZP: /* STY Zero Page */
            {
                store_operand_zeropage(cpu, cpu->y_lo);
            }
            break;

        case OP_STY_ZP_X: /* STY Zero Page, X */
            {
                store_operand_zeropage_x(cpu, cpu->y_lo);
            }
            break;
        
        case OP_STY_ABS: /* STY Absolute */
            {   
                store_operand_absolute(cpu, cpu->y_lo);
            }
            break;

        /* Transfer between registers --------------------------------- */



        case OP_TAX_IMP: /* TAX Implied */
            {
                op_transfer_to_x(cpu, cpu->a_lo);                
            }
            break;

        case OP_TAY_IMP: /* TAY Implied */
            {
                op_transfer_to_y(cpu, cpu->a_lo);
            }
            break;

        case OP_TYA_IMP: /* TYA Implied */
            {
                op_transfer_to_a(cpu, cpu->y_lo);
            }
            break;

        case OP_TSX_IMP: /* TSX Implied */
            {
                op_transfer_to_x(cpu, cpu->sp);
            }
            break;

        case OP_TXA_IMP: /* TXA Implied */
            {
                op_transfer_to_a(cpu, cpu->x_lo);
            }
            break;

        case OP_TXS_IMP: /* TXS Implied */
            {
                op_transfer_to_s(cpu, cpu->x_lo);
            }
            break;

        /* BRK --------------------------------- */
        case OP_BRK_IMP: /* BRK */
            {               
                push_word(cpu, cpu->pc+1); // pc of BRK plus 1 - leaves room for BRK 'mark'
                push_byte(cpu, cpu->p | FLAG_B | FLAG_UNUSED); // break flag and Unused bit set to 1.
                cpu->p |= FLAG_I; // interrupt disable flag set to 1.
                cpu->pc = read_word(cpu, BRK_VECTOR);
                if (DEBUG(DEBUG_OPERAND)) fprintf(stdout, " => $%04X", cpu->pc);
            }
            break;

        /* JMP --------------------------------- */
        case OP_JMP_ABS: /* JMP Absolute */
            {
                absaddr_t thisaddr = cpu->pc-1;
                cpu->pc = read_word_from_pc(cpu);
                if (thisaddr == cpu->pc) {
                    fprintf(stdout, " JUMP TO SELF INFINITE LOOP JMP $%04X\n", cpu->pc);
                    cpu->halt = HLT_INSTRUCTION;
                }
                if (DEBUG(DEBUG_OPERAND)) fprintf(stdout, " $%04X", cpu->pc);
            }
            break;

        case OP_JMP_IND: /* JMP (Indirect) */
            {
                absaddr_t addr = get_operand_address_absolute_indirect(cpu);
                cpu->pc = addr;
                if (DEBUG(DEBUG_OPERAND)) fprintf(stdout, " -> [$%04X]", addr);
            }
            break;

        /* JSR --------------------------------- */
        case OP_JSR_ABS: /* JSR Absolute */
            {
                absaddr_t addr = get_operand_address_absolute(cpu);
                push_word(cpu, cpu->pc -1); // return address pushed is last byte of JSR instruction
                cpu->pc = addr;
                if (DEBUG(DEBUG_OPERAND)) fprintf(stdout, "$%04X", cpu->pc);
            }
            break;

        /* RTI --------------------------------- */
        case OP_RTI_IMP: /* RTI */
            {
                // pop status register "ignore B | unused" which I think means don't change them.
                byte_t oldp = cpu->p & (FLAG_B | FLAG_UNUSED);
                byte_t p = pop_byte(cpu) & ~(FLAG_B | FLAG_UNUSED);
                cpu->p = p | oldp;

                cpu->pc = pop_word(cpu);
                if (DEBUG(DEBUG_OPERAND)) fprintf(stdout, "$%04X %02X", cpu->pc, cpu->p);
            }
            break;

        /* RTS --------------------------------- */
        case OP_RTS_IMP: /* RTS */
            {
                cpu->pc = pop_word(cpu);
                incr_cycles(cpu);
                cpu->pc++;
                incr_cycles(cpu);
                if (DEBUG(DEBUG_OPERAND)) fprintf(stdout, "$%04X", cpu->pc);
            }
            break;

        /* NOP --------------------------------- */
        case OP_NOP_IMP: /* NOP */
            {
                /* if (DEBUG) std::cout << "NOP"; */
                incr_cycles(cpu);
            }
            break;


        /* Flags ---------------------------------  */

        case OP_CLD_IMP: /* CLD Implied */
            {
                cpu->D = 0;
                incr_cycles(cpu);
            }
            break;

        case OP_SED_IMP: /* SED Implied */
            {
                cpu->D = 1;
                incr_cycles(cpu);
            }
            break;

        case OP_CLC_IMP: /* CLC Implied */
            {
                cpu->C = 0;
                incr_cycles(cpu);
            }
            break;

        case OP_CLI_IMP: /* CLI Implied */
            {
                cpu->I = 0;
                incr_cycles(cpu);
            }
            break;

        case OP_CLV_IMP: /* CLV */
            {
                cpu->V = 0;
                incr_cycles(cpu);
            }
            break;

        case OP_SEC_IMP: /* SEC Implied */
            {
                cpu->C = 1;
                incr_cycles(cpu);
            }
            break;

        case OP_SEI_IMP: /* SEI Implied */
            {
                cpu->I = 1;
                incr_cycles(cpu);
            }
            break;

        /** Misc --------------------------------- */

        case OP_BIT_ZP: /* BIT Zero Page */
            {
                byte_t N = get_operand_zeropage(cpu);
                byte_t temp = cpu->a_lo & N;
                set_n_z_v_flags(cpu, temp, N);
            }
            break;

        case OP_BIT_ABS: /* BIT Absolute */
            {
                byte_t N = get_operand_absolute(cpu);
                byte_t temp = cpu->a_lo & N;
                set_n_z_v_flags(cpu, temp, N);
            }
            break;

        /* End of Opcodes -------------------------- */

        /* Fake opcodes for testing -------------------------- */

        case OP_HLT_IMP: /* HLT */
            {
                cpu->halt = HLT_INSTRUCTION;
            }
            break;

        default:
            fprintf(stdout, "Unknown opcode: %04X: 0x%02X", cpu->pc-1, opcode);
            cpu->halt = HLT_INSTRUCTION;
    }
    if (DEBUG(DEBUG_OPCODE)) fprintf(stdout, "\n");

    return 0;
}

void run_cpus(void) {
    cpu_state *cpu = &CPUs[0];

    uint64_t last_display_update = 0;
    uint64_t last_5sec_update = 0;
    uint64_t last_5sec_cycles;

    while (1) {
        if (execute_next_6502(cpu) > 0) {
            break;
        }
        
        uint64_t current_time = get_current_time_in_microseconds();
        if (current_time - last_display_update > 16667) {
            event_poll(cpu); // they say call "once per frame"
            update_flash_state(cpu);
            update_display(cpu); // check for events 60 times per second.
            last_display_update = current_time;
        }

        if (current_time - last_5sec_update > 5000000) {
            uint64_t delta = cpu->cycles - last_5sec_cycles;
            fprintf(stdout, "%llu delta %llu cycles clock-mode: %d CPS: %f MHz [ slips: %llu, busy: %llu, sleep: %llu]\n", delta, cpu->cycles, cpu->clock_mode, (float)delta / float(5000000) , cpu->clock_slip, cpu->clock_busy, cpu->clock_sleep);
            last_5sec_cycles = cpu->cycles;
            last_5sec_update = current_time;
        }

        if (cpu->halt) {
            update_display(cpu); // update one last time to show the last state.
            break;
        }
    }
}


int main(int argc, char *argv[]) {
    std::cout << "Booting GSSquared!" << std::endl;

    int platform_id = 1;  // default to Apple II Plus
    int opt;
    
    while ((opt = getopt(argc, argv, "p:a:b:")) != -1) {
        switch (opt) {
            case 'p':
                platform_id = atoi(optarg);
                break;
            case 'a':
                loader_set_file_info(optarg, 0x0801);
                break;
            case 'b':
                loader_set_file_info(optarg, 0x7000);
                break;
            default:
                fprintf(stderr, "Usage: %s [-p platform] [-a program.bin] [-b loader.bin]\n", argv[0]);
                exit(1);
        }
    }

    init_cpus();

#if 0

        // this is the one test system.
        demo_ram();
#endif

#if 0
        // read file 6502_65C02_functional_tests/bin_files/6502_functional_test.bin into a 64k byte array

        uint8_t *memory_chunk = (uint8_t *)malloc(65536); // Allocate 64KB memory chunk
        if (memory_chunk == NULL) {
            fprintf(stderr, "Failed to allocate memory\n");
            exit(1);
        }

        FILE* file = fopen("6502_65C02_functional_tests/bin_files/6502_functional_test.bin", "rb");
        if (file == NULL) {
            fprintf(stderr, "Failed to open file\n");
            exit(1);
        }
        fread(memory_chunk, 1, 65536, file);
        fclose(file);
        for (uint64_t i = 0; i < 65536; i++) {
            raw_memory_write(&CPUs[0], i, memory_chunk[i]);
        }
#endif

#if 1
        rom_data *rd = load_platform_roms(platform_id);

        if (!rd) {
            fprintf(stdout, "Failed to load platform roms\n");
        }
        // Load into memory at correct address
        for (uint64_t i = 0; i < rd->main_size; i++) {
            raw_memory_write(&CPUs[0], rd->main_base_addr + i, rd->main_rom[i]);
        }
        // we could dispose of this now if we wanted..
#endif
printf("did we get here?\n");
    if (init_display_sdl(rd)) {
        fprintf(stderr, "Error initializing display\n");
        exit(1);
    }

    init_keyboard();
    init_device_display();
    init_speaker(&CPUs[0]);
    init_thunderclock(1);

    cpu_reset(&CPUs[0]);

    run_cpus();

    printf("CPU halted: %d\n", CPUs[0].halt);
    if (CPUs[0].halt == HLT_INSTRUCTION) { // keep screen up and give user a chance to see the last state.
        printf("Press Enter to continue...");
        getchar();
    }

    //dump_full_speaker_event_log();

    free_display();

    debug_dump_memory(&CPUs[0], 0x1230, 0x123F);
    return 0;
}
