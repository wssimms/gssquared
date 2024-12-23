#include <iostream>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <time.h>
#include <mach/mach_time.h>

#include "types.hpp"
#include "gs2.hpp"
#include "cpu.hpp"
#include "memory.hpp"
#include "opcodes.hpp"
#include "debug.hpp"
#include "clock.hpp"
#include "test.hpp"

// how many nanoseconds per second
constexpr uint64_t NS_PER_SECOND = /* 1000000000 */ 999999999; /* if we do 1 billion even it doesn't work. */

// target effective emulated mhz rate
constexpr uint64_t HZ_RATE = 2800000; /* 1 means 1 cycle per real second */

// how many nanoseconds per cycle
constexpr uint64_t nS_PER_CYCLE = NS_PER_SECOND / HZ_RATE;

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
 * Main components:
 *  - 6502 CPU emulation
 *  - 65c02 CPU emulation
 *  - 65sc816 CPU emulation (8 and 16-bit)
 *  - Display Emulation
 *     - Text - 40 / 80 col
 *     - Lores - 40 x 40, 40 x 48, 80 x 40, 80 x 48
 *     - Hires -  etc.
 *  - Disk I/O
 *   - 5.25 Emulation
 *   - 3.25 Emulation
 *   - SCSI Card Emulation
 *  - Memory management emulate a proposed MMU to allow multiple virtual CPUs to each have their own 16M address space
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
    
    for (int i = 0; i < RAM_SIZE / GS2_PAGE_SIZE; i++) {
        cpu->memory->page_info[i].type = MEM_RAM;
        cpu->memory->pages[i] = new memory_page(); /* do we care if this is aligned */
        if (!cpu->memory->pages[i]) {
            std::cerr << "Failed to allocate memory page " << i << std::endl;
            exit(1);
        }
    }
}

uint64_t get_current_time_in_microseconds() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::high_resolution_clock::now().time_since_epoch()).count();
}

void init_cpus() {
    for (int i = 0; i < MAX_CPUS; i++) {
        init_memory(&CPUs[i]);

        CPUs[i].boot_time = get_current_time_in_microseconds();
        CPUs[i].pc = 0x0100;
        CPUs[i].sp = 0;
        CPUs[i].a = 0;
        CPUs[i].x = 0;
        CPUs[i].y = 0;
        CPUs[i].p = 0;
        CPUs[i].cycles = 0;
        CPUs[i].last_tick = 0;
        CPUs[i].free_run = 1;

        // Get the conversion factor for mach_absolute_time to nanoseconds
        mach_timebase_info_data_t info;
        mach_timebase_info(&info);

        // Calculate time per cycle at emulated rate
        CPUs[i].cycle_duration_ns = 1000000000 / HZ_RATE;

        // Convert the cycle duration to mach_absolute_time() units
        CPUs[i].cycle_duration_ticks = (CPUs[i].cycle_duration_ns * info.denom / info.numer); // fudge
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

inline void set_n_z_v_flags(cpu_state *cpu, uint8_t value) {
    cpu->Z = (value == 0);
    cpu->N = (value & 0x80) != 0;
    cpu->V = (value & 0x40) != 0;
}


/**
 * perform accumulator addition. M is current value of accumulator. 
 * cpu: cpu flag
 * N: number being added to accumulator
 */
inline void add_and_set_flags(cpu_state *cpu, uint8_t N) {
    uint8_t M = cpu->a_lo;
    uint32_t S = M + N + (cpu->C);
    uint8_t S8 = (uint8_t) S;
    cpu->a_lo = S8;
    cpu->C = (S & 0x0100) >> 8;
    cpu->V =  !((M ^ N) & 0x80) && ((M ^ S8) & 0x80); // from 6502.org article https://www.righto.com/2012/12/the-6502-overflow-flag-explained.html?m=1
    set_n_z_flags(cpu, cpu->a_lo);
    if (DEBUG) std::cout << "   M: " << int_to_hex(M) << "  N: " << int_to_hex(N) 
            << "  S: " << int_to_hex(cpu->a_lo) << "  C: " << int_to_hex(cpu->C) << "  V: " << int_to_hex(cpu->V) ;
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
    if (DEBUG) std::cout << "   M: " << int_to_hex(M) << "  N: " << int_to_hex(N) 
            << "  S: " << int_to_hex(S8) << "  C: " << int_to_hex(cpu->C) << "  V: " << int_to_hex(cpu->V) ;
    return S8;
}

inline void subtract_and_set_flags(cpu_state *cpu, uint8_t N) {
    cpu->a_lo = subtract_core(cpu, cpu->a_lo, N, cpu->C);
}

/** Identical algorithm to subtraction mostly.
 * Compares are the same as SBC with the carry flag assumed to be 1.
 * does not store the result in the accumulator
 */
inline void compare_and_set_flags(cpu_state *cpu, uint8_t M, uint8_t N) {
    uint8_t dummy = subtract_core(cpu, M, N, 1);
}

/**
 * Good discussion of branch instructions:
 * https://www.masswerk.at/6502/6502_instruction_set.html#BCC
 */
inline void branch_if(cpu_state *cpu, uint8_t N, bool condition) {
    uint16_t oaddr = cpu->pc;
    uint16_t taddr = oaddr + N;
    if (DEBUG) std::cout << " => $" << int_to_hex(taddr);

    if (condition) {
        cpu->pc = cpu->pc + N;
        // branch taken uses another clock to update the PC
        incr_cycles(cpu); 
        /* If a branch is taken and the target is on a different page, this adds another CPU cycle (4 in total). */
        if ((oaddr & 0xFF00) != (taddr & 0xFF00)) {
            incr_cycles(cpu);
        }
        if (DEBUG) std::cout << " (taken)";
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
    if (DEBUG) std::cout << " $" << int_to_hex(zpaddr) ;
    return zpaddr;
}

inline zpaddr_t get_operand_address_zeropage_x(cpu_state *cpu) {
    zpaddr_t zpaddr = read_byte_from_pc(cpu);
    zpaddr_t taddr = zpaddr + cpu->x_lo; // make sure it wraps.
    if (DEBUG) std::cout << " $" << int_to_hex(zpaddr) << ",X" ;
    return taddr;
}

inline zpaddr_t get_operand_address_zeropage_y(cpu_state *cpu) {
    zpaddr_t zpaddr = read_byte_from_pc(cpu);
    zpaddr_t taddr = zpaddr + cpu->y_lo; // make sure it wraps.
    if (DEBUG) std::cout << " $" << int_to_hex(zpaddr) << ",Y" ;
    return taddr;
}

inline absaddr_t get_operand_address_absolute(cpu_state *cpu) {
    absaddr_t addr = read_word_from_pc(cpu);
    if (DEBUG) std::cout << " $" << int_to_hex(addr) ;
    return addr;
}

inline absaddr_t get_operand_address_absolute_x(cpu_state *cpu) {
    absaddr_t addr = read_word_from_pc(cpu);
    absaddr_t taddr = addr + cpu->x_lo;
    if ((addr & 0xFF00) != (taddr & 0xFF00)) {
        incr_cycles(cpu);
    }
    if (DEBUG) std::cout << " $" << int_to_hex(addr) << ",X" ; // can add effective address here.
    return taddr;
}

inline absaddr_t get_operand_address_absolute_y(cpu_state *cpu) {
    absaddr_t addr = read_word_from_pc(cpu);
    absaddr_t taddr = addr + cpu->y_lo;
    if ((addr & 0xFF00) != (taddr & 0xFF00)) {
        incr_cycles(cpu);
    }
    if (DEBUG) std::cout << " $" << int_to_hex(addr) << ",Y" ;
    return taddr;
}

inline uint16_t get_operand_address_indirect_x(cpu_state *cpu) {
    zpaddr_t zpaddr = read_byte_from_pc(cpu);
    absaddr_t taddr = read_word(cpu,zpaddr + cpu->x_lo);
    if (DEBUG) std::cout << " ($" << int_to_hex(zpaddr) << ",X)  -> $" << int_to_hex(taddr) ;
    return taddr;
}

inline absaddr_t get_operand_address_indirect_y(cpu_state *cpu) {
    zpaddr_t zpaddr = read_byte_from_pc(cpu);
    absaddr_t iaddr = read_word(cpu,zpaddr);
    absaddr_t taddr = iaddr + cpu->y_lo;

    if ((iaddr & 0xFF00) != (taddr & 0xFF00)) {
        incr_cycles(cpu);
    }
    if (DEBUG) std::cout << " ($" << int_to_hex(zpaddr) << "),Y  -> $" << int_to_hex(taddr) ;
    return taddr;
}

/** Second, these methods (get_operand_*) read the operand value from memory. */

inline byte_t get_operand_immediate(cpu_state *cpu) {
    byte_t N = read_byte_from_pc(cpu);
    if (DEBUG) std::cout << " #$" << int_to_hex(N) ;
    return N;
}

inline byte_t get_operand_zeropage(cpu_state *cpu) {
    zpaddr_t addr = get_operand_address_zeropage(cpu);
    byte_t N = read_byte(cpu, addr);
    if (DEBUG) std::cout << " -> #$" << int_to_hex(N) ;
    return N;
}

inline void store_operand_zeropage(cpu_state *cpu, byte_t N) {
    zpaddr_t addr = get_operand_address_zeropage(cpu);
    write_byte(cpu, addr, N);
    if (DEBUG) std::cout  << "   [#" << int_to_hex(N)  << "]";
}

inline byte_t get_operand_zeropage_x(cpu_state *cpu) {
    zpaddr_t addr = get_operand_address_zeropage_x(cpu);
    byte_t N = read_byte(cpu, addr);
    if (DEBUG) std::cout << " [#" << int_to_hex(N)  << "] <- $" << int_to_hex((zpaddr_t)addr);
    return N;
}

inline void store_operand_zeropage_x(cpu_state *cpu, byte_t N) {
    zpaddr_t addr = get_operand_address_zeropage_x(cpu);
    write_byte(cpu, addr, N);
    if (DEBUG) std::cout << "  [#" << int_to_hex(N)  << "] -> $" << int_to_hex((zpaddr_t)addr);
}

inline byte_t get_operand_zeropage_y(cpu_state *cpu) {
    zpaddr_t zpaddr = get_operand_address_zeropage_y(cpu);
    byte_t N = read_byte(cpu, zpaddr);
    if (DEBUG) std::cout << "   [#" << int_to_hex(N)  << "] <- $" << int_to_hex(zpaddr);
    return N;
}

inline void store_operand_zeropage_y(cpu_state *cpu, byte_t N) {
    zpaddr_t zpaddr = get_operand_address_zeropage_y(cpu);
    write_byte(cpu, zpaddr, N);
    if (DEBUG) std::cout << "  [#" << int_to_hex(N)  << "] -> $" << int_to_hex(zpaddr);
}

inline byte_t get_operand_zeropage_indirect_x(cpu_state *cpu) {
    absaddr_t taddr = get_operand_address_indirect_x(cpu);
    byte_t N = read_byte(cpu, taddr);
    if (DEBUG) std::cout << "  [#" << int_to_hex(N) << "] <- $" << int_to_hex(taddr);
    return N;
}

inline void store_operand_zeropage_indirect_x(cpu_state *cpu, byte_t N) {
    absaddr_t taddr = get_operand_address_indirect_x(cpu);
    write_byte(cpu, taddr, N);
    if (DEBUG) std::cout << "   [#" << int_to_hex(N) << "] <- $" << int_to_hex(taddr);
}

inline byte_t get_operand_zeropage_indirect_y(cpu_state *cpu) {
    absaddr_t addr = get_operand_address_indirect_y(cpu);
    byte_t N = read_byte(cpu, addr);
    if (DEBUG) std::cout << "  [#" << int_to_hex(N) << "] <- $" << int_to_hex(addr);
    return N;
}

inline void store_operand_zeropage_indirect_y(cpu_state *cpu, byte_t N) {
    absaddr_t addr = get_operand_address_indirect_y(cpu);
    write_byte(cpu, addr, N);
    if (DEBUG) std::cout << "   [#" << int_to_hex(N) << "] -> $" << int_to_hex(addr);
}

inline byte_t get_operand_absolute(cpu_state *cpu) {
    absaddr_t addr = get_operand_address_absolute(cpu);
    byte_t N = read_byte(cpu, addr);
    if (DEBUG) std::cout << "   [#" << int_to_hex(N) << "]";
    return N;
}

inline void store_operand_absolute(cpu_state *cpu, byte_t N) {
    absaddr_t addr = get_operand_address_absolute(cpu);
    write_byte(cpu, addr, N);
    if (DEBUG) std::cout << "   [#" << int_to_hex(N) << "]";
}

inline byte_t get_operand_absolute_x(cpu_state *cpu) {
    absaddr_t addr = get_operand_address_absolute_x(cpu);
    byte_t N = read_byte(cpu, addr);
    if (DEBUG) std::cout << "   [#" << int_to_hex(N) << "] <- $" << int_to_hex(addr);
    return N;
}

inline void store_operand_absolute_x(cpu_state *cpu, byte_t N) {
    absaddr_t addr = get_operand_address_absolute_x(cpu);
    write_byte(cpu, addr, N);
    if (DEBUG) std::cout << "   [#" << int_to_hex(N) << "] -> $" << int_to_hex(addr);
}

inline byte_t get_operand_absolute_y(cpu_state *cpu) {
    absaddr_t addr = get_operand_address_absolute_y(cpu);
    byte_t N = read_byte(cpu, addr);
    if (DEBUG) std::cout << "   [#" << int_to_hex(N) << "] <- $" << int_to_hex(addr);
    return N;
}

inline void store_operand_absolute_y(cpu_state *cpu, byte_t N) {
    absaddr_t addr = get_operand_address_absolute_y(cpu);
    write_byte(cpu, addr, N);
    if (DEBUG) std::cout << "   [#" << int_to_hex(N) << "] -> $" << int_to_hex(addr);
}

inline byte_t get_operand_relative(cpu_state *cpu) {
    byte_t N = read_byte_from_pc(cpu);
    if (DEBUG) std::cout << " #$" << int_to_hex(N);
    return N;
}


inline void op_transfer_to_x(cpu_state *cpu, byte_t N) {
    cpu->x_lo = N;
    set_n_z_flags(cpu, cpu->x_lo);
    incr_cycles(cpu);
    if (DEBUG) std::cout << "[#$" << int_to_hex(N) << "] -> X";
}

inline void op_transfer_to_y(cpu_state *cpu, byte_t N) {
    cpu->y_lo = N;
    set_n_z_flags(cpu, cpu->y_lo);
    incr_cycles(cpu);
    if (DEBUG) std::cout << "[#$" << int_to_hex(N) << "] -> Y";
}

inline void op_transfer_to_a(cpu_state *cpu, byte_t N) {
    cpu->a_lo = N;
    set_n_z_flags(cpu, cpu->a_lo);
    incr_cycles(cpu);
    if (DEBUG) std::cout << "[#$" << int_to_hex(N) << "] -> A";
}

inline void op_transfer_to_s(cpu_state *cpu, byte_t N) {
    cpu->sp = N;
    incr_cycles(cpu);
    if (DEBUG) std::cout << "[#$" << int_to_hex(N) << "] -> S";
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
    if (DEBUG) std::cout << "   [#" << int_to_hex(N) << "]";
}

inline void dec_operand_zeropage(cpu_state *cpu) {
    zpaddr_t zpaddr = get_operand_address_zeropage(cpu);
    if (DEBUG) std::cout << " $" << int_to_hex(zpaddr) ;
    dec_operand(cpu, zpaddr);
}

inline void dec_operand_zeropage_x(cpu_state *cpu) {
    zpaddr_t zpaddr = get_operand_address_zeropage_x(cpu);
    if (DEBUG) std::cout << " $" << int_to_hex(zpaddr) ;
    dec_operand(cpu, zpaddr);
}

inline void dec_operand_absolute(cpu_state *cpu) {
    absaddr_t addr = get_operand_address_absolute(cpu);
    if (DEBUG) std::cout << " $" << int_to_hex(addr) ;
    dec_operand(cpu, addr);
}

inline void dec_operand_absolute_x(cpu_state *cpu) {
    absaddr_t addr = get_operand_address_absolute_x(cpu);
    if (DEBUG) std::cout << " $" << int_to_hex(addr) ;
    dec_operand(cpu, addr);
}


inline byte_t logical_shift_right(cpu_state *cpu, byte_t N) {
    uint8_t C = N & 0x01;
    N = N >> 1;
    cpu->C = C;
    set_n_z_flags(cpu, N);
    if (DEBUG) std::cout << " [#" << int_to_hex(N) << "]";
    return N;
}

inline byte_t logical_shift_right_addr(cpu_state *cpu, absaddr_t addr) {
    byte_t N = read_byte(cpu, addr);
    byte_t result = logical_shift_right(cpu, N);
    write_byte(cpu, addr, result);
    if (DEBUG) std::cout << " -> $" << int_to_hex(addr);
    return result;
}

inline byte_t arithmetic_shift_left(cpu_state *cpu, byte_t N) {
    uint8_t C = (N & 0x80) >> 7;
    N = N << 1;
    cpu->C = C;
    set_n_z_flags(cpu, N);
    if (DEBUG) std::cout << " [#" << int_to_hex(N) << "]";
    return N;
}

inline byte_t arithmetic_shift_left_addr(cpu_state *cpu, absaddr_t addr) {
    byte_t N = read_byte(cpu, addr);
    byte_t result = arithmetic_shift_left(cpu, N);
    write_byte(cpu, addr, result);
    if (DEBUG) std::cout << " -> $" << int_to_hex(addr);
    return result;
}

inline void push_byte(cpu_state *cpu, byte_t N) {
    write_byte(cpu, cpu->sp, N);
    cpu->sp--;
    incr_cycles(cpu);
    if (DEBUG) std::cout << " [#" << int_to_hex(N) << "] -> S[" << int_to_hex((uint8_t) (cpu->sp + 1)) << "]";
}

inline void push_word(cpu_state *cpu, word_t N) {
    write_byte(cpu, cpu->sp, (N & 0xFF00) >> 8);
    write_byte(cpu, cpu->sp - 1, N & 0x00FF);
    cpu->sp -= 2;
    incr_cycles(cpu);
    if (DEBUG) std::cout << " [#" << int_to_hex(N) << "] -> S[" << int_to_hex((uint8_t) (cpu->sp + 1)) << "]";
}


int execute_next_6502(cpu_state *cpu) {
    int exit_code = 0;

    if (DEBUG) {
        uint64_t current_time = get_current_time_in_microseconds();
        std::cout << "[ " << cpu->cycles << " ]";
        uint64_t elapsed_time = current_time - cpu->boot_time;
        std::cout << "[eTime: " << elapsed_time << "] ";
        float_t cycles_per_second = (cpu->cycles * 1000000000.0) / (elapsed_time * 1000.0);
        std::cout << "[eHz: " << cycles_per_second << "] ";
    }

    if ( DEBUG == 0  && cpu->cycles > (10 * HZ_RATE) ) { // should take about 10 seconds.
        uint64_t current_time = get_current_time_in_microseconds();
        uint64_t elapsed_time = current_time - cpu->boot_time;
        std::cout << "[eTime: " << elapsed_time << "] ";
        float_t cycles_per_second = (cpu->cycles * 1000000000.0) / (elapsed_time * 1000.0);
        std::cout << "[eHz: " << cycles_per_second << "] ";
        exit(0);
    }

    if (DEBUG) std::cout << int_to_hex(cpu->pc); // so PC is correct.
    opcode_t opcode = read_byte_from_pc(cpu);
    if (DEBUG) std::cout << ": "  << get_opcode_name(opcode);

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

    /* Logical operations --------------------------------- */

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
                exit_code = 1;
                cpu->pc = read_word(cpu, BRK_VECTOR);
            }
            break;

        /* JMP --------------------------------- */
        case OP_JMP_ABS: /* JMP Absolute */
            {
                cpu->pc = read_word_from_pc(cpu);
                if (DEBUG) std::cout << " $" << int_to_hex(cpu->pc);
            }
            break;

        /* NOP --------------------------------- */
        case OP_NOP_IMP: /* NOP */
            {
                if (DEBUG) std::cout << "NOP";
                incr_cycles(cpu);
            }
            break;


        /* Flags ---------------------------------  */

        case OP_CLC_IMP: /* CLC Implied */
            {
                cpu->C = 0;
            }
            break;

        case OP_CLI_IMP: /* CLI Implied */
            {
                cpu->I = 0;
            }
            break;

        case OP_CLV_IMP: /* CLV */
            {
                cpu->V = 0;
            }
            break;


        case OP_SEC_IMP: /* SEC Implied */
            {
                cpu->C = 1;
            }
            break;

        case OP_SEI_IMP: /* SEI Implied */
            {
                cpu->I = 1;
            }
            break;

        /** Misc --------------------------------- */

        case OP_BIT_ZP: /* BIT Zero Page */
            {
                byte_t N = get_operand_zeropage(cpu);
                byte_t temp = cpu->a_lo & N;
                set_n_z_v_flags(cpu, temp);
            }
            break;

        case OP_BIT_ABS: /* BIT Absolute */
            {
                byte_t N = get_operand_absolute(cpu);
                byte_t temp = cpu->a_lo & N;
                set_n_z_v_flags(cpu, temp);
            }
            break;

       /* End of Opcodes -------------------------- */


        default:
            std::cout << "Unknown opcode: 0x" << int_to_hex(opcode);
            exit(1);
    }
    if (DEBUG) std::cout << std::endl;

    return exit_code;
}

void run_cpus(void) {
    cpu_state *cpu = &CPUs[0];
    cpu->pc = read_word(cpu, RESET_VECTOR);
    while (1) {
        if (execute_next_6502(cpu) > 0) {
            break;
        }
    }
}


int main() {
    int a = 2;

    std::cout << "Booting GSSquared!" << std::endl;
    init_cpus();
    demo_ram();
    /* run_cpus(); */
    debug_dump_memory(&CPUs[0], 0x1230, 0x123F);
    return 0;
}
