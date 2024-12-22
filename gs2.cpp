#include <iostream>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <time.h>
#include <mach/mach_time.h>

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
 * gssquared
 * 
 * Apple II/+/e/c/GS Emulator Extraordinaire
 * 
 * Main components:
 *  - 65sc816 CPU emulation
 *  - 65c02 CPU emulation
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

inline void set_n_z_flags(cpu_state *cpu, uint8_t value) {
    cpu->Z = (value == 0);
    cpu->N = (value & 0x80) != 0;
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
    cpu->V =  !((M ^ N) & 0x80) && ((M ^ S8) & 0x80); // from 6502.org article
    set_n_z_flags(cpu, cpu->a_lo);
    if (DEBUG) std::cout << "   M: " << int_to_hex(M) << "  N: " << int_to_hex(N) 
            << "  S: " << int_to_hex(cpu->a_lo) << "  C: " << int_to_hex(cpu->C) << "  V: " << int_to_hex(cpu->V) ;
}

/**
 * Handle addressing modes - different ways of fetching data for operations
 */
inline uint8_t get_operand_immediate(cpu_state *cpu) {
    uint8_t N = read_byte_from_pc(cpu);
    if (DEBUG) std::cout << " #$" << int_to_hex(N) ;
    return N;
}

inline uint8_t get_operand_zeropage(cpu_state *cpu) {
    uint8_t zpaddr = read_byte_from_pc(cpu);
    uint8_t N = read_byte(cpu, zpaddr);
    if (DEBUG) std::cout << " #$" << int_to_hex(N) ;
    return N;
}

inline uint8_t get_operand_zeropage_x(cpu_state *cpu) {
    uint8_t zpaddr = read_byte_from_pc(cpu);
    uint8_t taddr = (zpaddr + cpu->x_lo);
    uint8_t N = read_byte(cpu, taddr);
    if (DEBUG) std::cout << " $" << int_to_hex(zpaddr) << ",X   [#" << int_to_hex(N)  << "] <- $" << int_to_hex(taddr);
    return N;
}

inline uint8_t get_operand_zeropage_y(cpu_state *cpu) {
    uint8_t zpaddr = read_byte_from_pc(cpu);
    uint8_t taddr = (zpaddr + cpu->y_lo);
    uint8_t N = read_byte(cpu, taddr);
    if (DEBUG) std::cout << " $" << int_to_hex(zpaddr) << ",Y   [#" << int_to_hex(N)  << "] <- $" << int_to_hex(taddr);
    return N;
}

inline uint8_t get_operand_zeropage_indirect_x(cpu_state *cpu) {
    uint8_t zpaddr = read_byte_from_pc(cpu);
    uint16_t taddr = read_word(cpu,zpaddr + cpu->x_lo);
    uint8_t N = read_byte(cpu, taddr);
    if (DEBUG) std::cout << " ($" << int_to_hex(zpaddr) << ",X)   [#" << int_to_hex(N) << "] <- $" << int_to_hex(taddr);
    return N;
}

inline uint8_t get_operand_zeropage_indirect_y(cpu_state *cpu) {
    uint8_t zpaddr = read_byte_from_pc(cpu);
    uint16_t addr = read_word(cpu,zpaddr);
    uint16_t taddr = addr + cpu->y_lo;
    uint8_t N = read_byte(cpu, taddr);
    /* if (DEBUG) std::cout << " $(" << int_to_hex(zpaddr) << "),Y   [#" << int_to_hex(N)  << "]"; */
    if (DEBUG) std::cout << " ($" << int_to_hex(zpaddr) << "),Y   [#" << int_to_hex(N) << "] <- $" << int_to_hex(taddr) << " <- $" << int_to_hex(addr);

    return N;
}

inline uint8_t get_operand_absolute(cpu_state *cpu) {
    uint16_t abs_addr = read_word_from_pc(cpu);
    uint8_t N = read_byte(cpu, abs_addr);
    if (DEBUG) std::cout << " $" << int_to_hex(abs_addr) << "   [#" << int_to_hex(N) << "]";
    return N;
}

inline uint8_t get_operand_absolute_x(cpu_state *cpu) {
    uint16_t addr = read_word_from_pc(cpu);
    uint16_t taddr = addr + cpu->x_lo;
    uint8_t N = read_byte(cpu, taddr);
    if (DEBUG) std::cout << " $" << int_to_hex(addr) << ",X   [#" << int_to_hex(N) << "] <- $" << int_to_hex(taddr);
    return N;
}

inline uint8_t get_operand_absolute_y(cpu_state *cpu) {
    uint16_t addr = read_word_from_pc(cpu);
    uint16_t taddr = addr + cpu->y_lo;
    uint8_t N = read_byte(cpu, taddr);
    if (DEBUG) std::cout << " $" << int_to_hex(addr) << ",Y   [#" << int_to_hex(N) << "] <- $" << int_to_hex(taddr);
    return N;
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

    uint8_t opcode = read_byte_from_pc(cpu);

    if (DEBUG) std::cout << int_to_hex(cpu->pc) << ": "  << get_opcode_name(opcode);

    switch (opcode) {

        /* ADC --------------------------------- */
        case OP_ADC_IMM: /* ADC Immediate */
            {
                uint8_t M = cpu->a_lo; // save old value for display purposes below

                uint8_t N = get_operand_immediate(cpu);
                add_and_set_flags(cpu, N);                    
            }
            break;

        case OP_ADC_ZP: /* ADC ZP */
            {
                uint8_t M = cpu->a_lo;

                uint8_t N = get_operand_zeropage(cpu);
                add_and_set_flags(cpu, N);

/*                 if (DEBUG) std::cout << "ADC $" << int_to_hex(N) << "   M: " << int_to_hex(M) << "  N: " << int_to_hex(N) 
                    << "  S: " << int_to_hex(cpu->a_lo) << "  C: " << int_to_hex(cpu->C) << "  V: " << int_to_hex(cpu->V) << std::endl; */
            }
            break;


    /* AND --------------------------------- */

        case OP_AND_IMM: /* AND Immediate */
            {
                uint8_t N = get_operand_immediate(cpu);
                cpu->a_lo &= N;
                set_n_z_flags(cpu, cpu->a_lo);
                /* if (DEBUG) std::cout << "AND #$" << int_to_hex(N) << std::endl; */
            }
            break;

        case OP_AND_ZP: /* AND Zero Page */
            {
                uint8_t N = get_operand_zeropage(cpu);
                cpu->a_lo &= N;
                set_n_z_flags(cpu, cpu->a_lo);
                if (DEBUG) std::cout << "   [#" << int_to_hex(cpu->a_lo)  << "]" ;
            }
            break;

        case OP_AND_ZP_X: /* AND Zero Page, X */
            {
                uint8_t N = get_operand_zeropage_x(cpu);

                uint8_t addr, taddr;
                addr = read_byte_from_pc(cpu);
                taddr =  (addr + cpu->x_lo) & 0xFF;
                cpu->a_lo &= read_byte(cpu, taddr);
                set_n_z_flags(cpu, cpu->a_lo);
                if (DEBUG) std::cout << "AND $" << int_to_hex(addr) << ",X   [#" << int_to_hex(cpu->a_lo)  << "] <- $" << int_to_hex(taddr);
            }
            break;

        case OP_AND_ABS: /* AND Absolute */
            {
                uint16_t abs_addr = read_word_from_pc(cpu);
                cpu->a_lo &= read_byte(cpu, abs_addr);
                set_n_z_flags(cpu, cpu->a_lo);

                if (DEBUG) std::cout << "AND $" << int_to_hex(abs_addr) << "   [#" << int_to_hex(cpu->a_lo) << "]";
            }
            break;
        
        case OP_AND_ABS_X: /* AND Absolute, X */
            {
                uint16_t addr,taddr;
                addr = read_word_from_pc(cpu);
                taddr = addr + cpu->x_lo;
                cpu->a_lo &= read_byte(cpu, taddr);
                set_n_z_flags(cpu, cpu->a_lo);

                if (DEBUG) std::cout << "AND $" << int_to_hex(addr) << ",X   [#" << int_to_hex(cpu->a_lo) << "] <- $" << int_to_hex(taddr);
            }
            break;

        case OP_AND_ABS_Y: /* AND Absolute, Y */
            {
                uint16_t addr,taddr;
                addr = read_word_from_pc(cpu);
                taddr = addr + cpu->y_lo;
                cpu->a_lo &= read_byte(cpu, taddr);
                set_n_z_flags(cpu, cpu->a_lo);

                if (DEBUG) std::cout << "AND $" << int_to_hex(addr) << ",Y   [#" << int_to_hex(cpu->a_lo) << "] <- $" << int_to_hex(taddr);
            }
            break;

        case OP_AND_IND_X: /* AND (Indirect, X) */
            {
                uint8_t addr;
                uint8_t taddr;
                uint16_t indir;

                addr = read_byte_from_pc(cpu);
                taddr = addr + cpu->x_lo;
                indir = read_word(cpu, taddr);
                cpu->a_lo &= read_byte(cpu, indir);
                set_n_z_flags(cpu, cpu->a_lo);

                if (DEBUG) std::cout << "AND ($" << int_to_hex(addr) << ",X)   [#" << int_to_hex(cpu->a_lo) << "] <- $" << int_to_hex(indir) << " <- $" << int_to_hex(taddr);
            }
            break;

        case OP_AND_IND_Y: /* AND (Indirect), Y */
            {
                uint8_t addr;
                uint16_t taddr;
                uint16_t indir;

                addr = read_byte_from_pc(cpu);
                taddr = read_word(cpu, addr);
                indir = taddr + cpu->y_lo;
                
                cpu->a_lo &= read_byte(cpu, indir);
                set_n_z_flags(cpu, cpu->a_lo);

                if (DEBUG) std::cout << "AND ($" << int_to_hex(addr) << "),Y   [#" << int_to_hex(cpu->a_lo) << "] <- $" << int_to_hex(indir) << " <- $" << int_to_hex(taddr);
            }
            break;

        /* DE(xy) --------------------------------- */

        case OP_DEX_IMP: /* DEX Implied */
            {
                cpu->x_lo --;
                incr_cycles(cpu);
                set_n_z_flags(cpu, cpu->x_lo);
                /* if (DEBUG) std::cout << "DEX"; */
            }
            break;

        case OP_DEY_IMP: /* DEY Implied */
            {
                cpu->y_lo --;
                incr_cycles(cpu);
                set_n_z_flags(cpu, cpu->y_lo);
                /* if (DEBUG) std::cout << "DEY"; */
            }
            break;


        /* EOR --------------------------------- */

        case OP_EOR_IMM: /* EOR Immediate */
            {
                uint8_t N = get_operand_immediate(cpu);
                cpu->a_lo ^= N;
                set_n_z_flags(cpu, cpu->a_lo);
                /* if (DEBUG) std::cout << "EOR #$" << int_to_hex(N); */
            }
            break;

        case OP_EOR_ZP: /* EOR Zero Page */
            {
                uint8_t N = get_operand_zeropage(cpu);
                cpu->a_lo ^= N;
                set_n_z_flags(cpu, cpu->a_lo);
                /* if (DEBUG) std::cout << "EOR $" << int_to_hex(zpaddr) << "   [#" << int_to_hex(cpu->a_lo)  << "]"; */
            }
            break;

        case OP_EOR_ZP_X: /* EOR Zero Page, X */
            {
                uint8_t N = get_operand_zeropage_x(cpu);
                cpu->a_lo ^= N;
                set_n_z_flags(cpu, cpu->a_lo);
                /* if (DEBUG) std::cout << "EOR $" << int_to_hex(zpaddr) << ",X   [#" << int_to_hex(cpu->a_lo)  << "] <- $" << int_to_hex(taddr); */
            }
            break;

        case OP_EOR_ABS: /* EOR Absolute */
            {
                uint8_t N = get_operand_absolute(cpu);
                cpu->a_lo ^= N;
                set_n_z_flags(cpu, cpu->a_lo);

                /* if (DEBUG) std::cout << "EOR $" << int_to_hex(abs_addr) << "   [#" << int_to_hex(cpu->a_lo) << "]"; */
            }
            break;
        
        case OP_EOR_ABS_X: /* EOR Absolute, X */
            {
                uint8_t N = get_operand_absolute_x(cpu);
                cpu->a_lo ^= N;
                set_n_z_flags(cpu, cpu->a_lo);

                /* if (DEBUG) std::cout << "EOR $" << int_to_hex(addr) << ",X   [#" << int_to_hex(cpu->a_lo) << "] <- $" << int_to_hex(taddr); */
            }
            break;

        case OP_EOR_ABS_Y: /* EOR Absolute, Y */
            {
                uint8_t N = get_operand_absolute_y(cpu);
                cpu->a_lo ^= N;
                set_n_z_flags(cpu, cpu->a_lo);

                /* if (DEBUG) std::cout << "EOR $" << int_to_hex(addr) << ",Y   [#" << int_to_hex(cpu->a_lo) << "] <- $" << int_to_hex(taddr); */
            }
            break;

        case OP_EOR_IND_X: /* EOR (Indirect, X) */
            {
                uint8_t N = get_operand_zeropage_indirect_x(cpu);
                cpu->a_lo ^= N;
                set_n_z_flags(cpu, cpu->a_lo);

                /* if (DEBUG) std::cout << "EOR ($" << int_to_hex(addr) << ",X)   [#" << int_to_hex(cpu->a_lo) << "] <- $" << int_to_hex(indir) << " <- $" << int_to_hex(taddr); */
            }
            break;

        case OP_EOR_IND_Y: /* EOR (Indirect), Y */
            {
                uint8_t N = get_operand_zeropage_indirect_y(cpu);
                cpu->a_lo ^= N;
                set_n_z_flags(cpu, cpu->a_lo);

                /* if (DEBUG) std::cout << "EOR ($" << int_to_hex(addr) << "),Y   [#" << int_to_hex(cpu->a_lo) << "] <- $" << int_to_hex(indir) << " <- $" << int_to_hex(taddr); */
            }
            break;

        /* IN(xy) --------------------------------- */

        case OP_INX_IMP: /* INX Implied */
            {
                cpu->x_lo ++;
                incr_cycles(cpu);
                set_n_z_flags(cpu, cpu->x_lo);
                /* if (DEBUG) std::cout << "INX"; */
            }
            break;

        case OP_INY_IMP: /* INY Implied */
            {
                cpu->y_lo ++;
                incr_cycles(cpu);
                set_n_z_flags(cpu, cpu->y_lo);
                /* if (DEBUG) std::cout << "INY"; */
            }
            break;




        /* LDA --------------------------------- */

        case OP_LDA_IMM: /* LDA Immediate */
            {
                cpu->a_lo = get_operand_immediate(cpu);
                set_n_z_flags(cpu, cpu->a_lo);
                /* if (DEBUG) std::cout << "LDA #$" << int_to_hex(cpu->a_lo); */
            }
            break;

        case OP_LDA_ZP: /* LDA Zero Page */
            {
                cpu->a_lo =  get_operand_zeropage(cpu);
                set_n_z_flags(cpu, cpu->a_lo);
                /* if (DEBUG) std::cout << "LDA $" << int_to_hex(zpaddr) << "   [#" << int_to_hex(cpu->a_lo)  << "]"; */
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
/*                 if (DEBUG) std::cout << "LDA ($" << int_to_hex(addr) << ",X)   [#" << int_to_hex(cpu->a_lo) << "] <- $" << int_to_hex(indir) << " <- $" << int_to_hex(taddr); */
            }
            break;

        case OP_LDA_IND_Y: /* LDA (Indirect), Y */
            {
                cpu->a_lo = get_operand_zeropage_indirect_y(cpu);
                set_n_z_flags(cpu, cpu->a_lo);
/*                 if (DEBUG) std::cout << "LDA ($" << int_to_hex(addr) << "),Y   [#" << int_to_hex(cpu->a_lo) << "] <- $" << int_to_hex(indir) << " <- $" << int_to_hex(taddr); */
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
                uint8_t N = get_operand_immediate(cpu);
                cpu->y_lo = N;
                set_n_z_flags(cpu, cpu->y_lo);
                /* if (DEBUG) std::cout << "LDY #$" << int_to_hex(cpu->y_lo); */
            }
            break;
        
        case OP_LDY_ZP: /* LDY Zero Page */
            {
                uint8_t N = get_operand_zeropage(cpu);
                cpu->y_lo = N;
                set_n_z_flags(cpu, cpu->y_lo);
            }
            break;

        case OP_LDY_ZP_X: /* LDY Zero Page, X */
            {
                uint8_t N = get_operand_zeropage_x(cpu);
                cpu->y_lo = N;
                set_n_z_flags(cpu, cpu->y_lo);
            }
            break;

        case OP_LDY_ABS: /* LDY Absolute */
            {
                uint8_t N = get_operand_absolute(cpu);
                cpu->y_lo = N;
                set_n_z_flags(cpu, cpu->y_lo);
            }
            break;

        case OP_LDY_ABS_X: /* LDY Absolute, X */
            {
                uint8_t N = get_operand_absolute_x(cpu);
                cpu->y_lo = N;
                set_n_z_flags(cpu, cpu->y_lo);
            }
            break;

    /* ORA --------------------------------- */

        case OP_ORA_IMM: /* AND Immediate */
            {
                uint8_t N = get_operand_immediate(cpu);
                cpu->a_lo |= N;
                set_n_z_flags(cpu, cpu->a_lo);
                /* if (DEBUG) std::cout << "ORA #$" << int_to_hex(N); */
            }
            break;

        case OP_ORA_ZP: /* AND Zero Page */
            {
                uint8_t N = get_operand_zeropage(cpu);
                cpu->a_lo |= N;
                set_n_z_flags(cpu, cpu->a_lo);
                /* if (DEBUG) std::cout << "ORA $" << int_to_hex(zpaddr) << "   [#" << int_to_hex(cpu->a_lo)  << "]"; */
            }
            break;

        case OP_ORA_ZP_X: /* AND Zero Page, X */
            {
                uint8_t N = get_operand_zeropage_x(cpu);
                cpu->a_lo |= N;
                set_n_z_flags(cpu, cpu->a_lo);
                /* if (DEBUG) std::cout << "ORA $" << int_to_hex(zpaddr) << ",X   [#" << int_to_hex(cpu->a_lo)  << "] <- $" << int_to_hex(taddr); */
            }
            break;

        case OP_ORA_ABS: /* AND Absolute */
            {
                uint8_t N = get_operand_absolute(cpu);
                cpu->a_lo |= N;
                set_n_z_flags(cpu, cpu->a_lo);
                /* if (DEBUG) std::cout << "ORA $" << int_to_hex(abs_addr) << "   [#" << int_to_hex(cpu->a_lo) << "]"; */
            }
            break;
        
        case OP_ORA_ABS_X: /* AND Absolute, X */
            {
                uint8_t N = get_operand_absolute_x(cpu);
                cpu->a_lo |= N;
                set_n_z_flags(cpu, cpu->a_lo);
                /* if (DEBUG) std::cout << "LDA $" << int_to_hex(addr) << ",X   [#" << int_to_hex(cpu->a_lo) << "] <- $" << int_to_hex(taddr); */
            }
            break;

        case OP_ORA_ABS_Y: /* AND Absolute, Y */
            {
                uint8_t N = get_operand_absolute_y(cpu);
                cpu->a_lo |= N;
                set_n_z_flags(cpu, cpu->a_lo);
                /* if (DEBUG) std::cout << "LDA $" << int_to_hex(addr) << ",X   [#" << int_to_hex(cpu->a_lo) << "] <- $" << int_to_hex(taddr); */
            }
            break;

        case OP_ORA_IND_X: /* AND (Indirect, X) */
            {
                uint8_t N = get_operand_zeropage_indirect_x(cpu);
                cpu->a_lo |= N;
                set_n_z_flags(cpu, cpu->a_lo);
/*                 if (DEBUG) std::cout << "ORA ($" << int_to_hex(addr) << ",X)   [#" << int_to_hex(cpu->a_lo) << "] <- $" << int_to_hex(indir) << " <- $" << int_to_hex(taddr); */
            }
            break;

        case OP_ORA_IND_Y: /* AND (Indirect), Y */
            {
                uint8_t N = get_operand_zeropage_indirect_y(cpu);
                cpu->a_lo |= N;
                set_n_z_flags(cpu, cpu->a_lo);

                /* if (DEBUG) std::cout << "ORA ($" << int_to_hex(addr) << "),Y   [#" << int_to_hex(cpu->a_lo) << "] <- $" << int_to_hex(indir) << " <- $" << int_to_hex(taddr); */
            }
            break;

        /* STA --------------------------------- */
        case OP_STA_ZP: /* STA Zero Page */
            {
                uint8_t zpaddr = read_byte_from_pc(cpu);
                write_byte(cpu, zpaddr, cpu->a_lo);
                if (DEBUG) std::cout << " $" << int_to_hex(zpaddr)  << "   [#" << int_to_hex(cpu->a_lo)  << "]";
            }
            break;

        case OP_STA_ABS: /* STA Absolute */
            {
                uint16_t abs_addr = read_word_from_pc(cpu);
                write_byte(cpu, abs_addr, cpu->a_lo);
                if (DEBUG) std::cout << " $" << int_to_hex(abs_addr) << "   [#" << int_to_hex(cpu->a_lo) << "]";
            }
            break;

        /* STX --------------------------------- */
        case OP_STX_ZP: /* STX Zero Page */
            {
                uint8_t zpaddr = read_byte_from_pc(cpu);
                write_byte(cpu, zpaddr, cpu->x_lo);
                if (DEBUG) std::cout << " $" << int_to_hex(zpaddr)  << "   [#" << int_to_hex(cpu->x_lo)  << "]";
            }
            break;

        case OP_STX_ABS: /* STX Absolute */
            {
                uint16_t abs_addr = read_word_from_pc(cpu);
                write_byte(cpu, abs_addr, cpu->x_lo);
                if (DEBUG) std::cout << " $" << int_to_hex(abs_addr);
            }
            break;

    /* STY --------------------------------- */
        case OP_STY_ZP: /* STY Zero Page */
            {
                uint8_t zpaddr = read_byte_from_pc(cpu);
                write_byte(cpu, zpaddr, cpu->y_lo);
                if (DEBUG) std::cout << " $" << int_to_hex(zpaddr)  << "   [#" << int_to_hex(cpu->y_lo)  << "]";
            }
            break;
        
        case OP_STY_ABS: /* STY Absolute */
            {   
                uint16_t abs_addr = read_word_from_pc(cpu);
                write_byte(cpu, abs_addr, cpu->y_lo);
                if (DEBUG) std::cout << " $" << int_to_hex(abs_addr);
            }
            break;

        /* BRK --------------------------------- */
        case OP_BRK_IMP: /* BRK */
            {
                exit_code = 1;
                cpu->pc = read_word(cpu, BRK_VECTOR);
                /* if (DEBUG) std::cout << "BRK"; */
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
                /* if (DEBUG) std::cout << "CLC"; */
            }
            break;

        case OP_CLI_IMP: /* CLI Implied */
            {
                cpu->I = 0;
                /* if (DEBUG) std::cout << "CLI"; */
            }
            break;

        case OP_CLV_IMP: /* CLV */
            {
                cpu->V = 0;
                /* if (DEBUG) std::cout << "CLV"; */
            }
            break;


        case OP_SEC_IMP: /* SEC Implied */
            {
                cpu->C = 1;
                /* if (DEBUG) std::cout << "SEC"; */
            }
            break;

        case OP_SEI_IMP: /* SEI Implied */
            {
                cpu->I = 1;
                /* if (DEBUG) std::cout << "SEI"; */
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
