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

#include <cstdio>
#include <time.h>
#include <cstdlib>
#include <iostream>

#include "cpu.hpp"
#include "memory.hpp"

clock_mode_info_t clock_mode_info[NUM_CLOCK_MODES] = {
    { 0, 0, 17008 },
    { 1020500, (1000000000 / 1020500)+1, 17008 },
    { 2800000, (1000000000 / 2800000), 46666 },
    { 4000000, (1000000000 / 4000000), 66665 }
};

/* uint64_t HZ_RATES[] = {
    0,
    4000000,
    2800000,
    1020500
};

// 1000000000 / cpu->HZ_RATE
uint64_t cycle_durations_ns[] = {
    0, // free run
    1000000000 / 4000000, // Apple //c+
    1000000000 / 2800000, // 2.8MHz IIGS,
    (1000000000 / 1020500)+1, // 1020500, Apple II+/IIe - +1 for rounding error, it's actually 979.9
};

uint64_t cycles_per_burst[] = { 17008, 66665, 46666, 17008 };
 */

void set_clock_mode(cpu_state *cpu, clock_mode_t mode) {
    // Get the conversion factor for mach_absolute_time to nanoseconds
/*     mach_timebase_info_data_t info;
    mach_timebase_info(&info);
 */
    // TODO: if this is ever called from inside a CPU loop, we need to exit that loop
    // immediately in order to avoid weird calculations around.
    // So add a "speedshift" cpu flag.

    cpu->HZ_RATE = clock_mode_info[mode].hz_rate;
    // Lookup time per emulated cycle
    cpu->cycle_duration_ns = clock_mode_info[mode].cycle_duration_ns;

    cpu->clock_mode = mode;
    fprintf(stdout, "Clock mode: %d HZ_RATE: %llu cycle_duration_ns: %llu \n", cpu->clock_mode, cpu->HZ_RATE, cpu->cycle_duration_ns);
}

/* void set_clock_mode(cpu_state *cpu, clock_mode mode) {
    // Get the conversion factor for mach_absolute_time to nanoseconds
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);

    cpu->HZ_RATE = HZ_RATES[mode];
    // Calculate time per cycle at emulated rate
    cpu->cycle_duration_ns = cycle_durations_ns[mode];
    cpu->cycle_duration_ticks = cycle_durations_ticks[mode];

    // Convert the cycle duration to mach_absolute_time() units
    //cpu->cycle_duration_ticks = (cpu->cycle_duration_ns * info.denom / info.numer); // fudge

    cpu->clock_mode = mode;
    fprintf(stdout, "Clock mode: %d HZ_RATE: %llu cycle_duration_ns: %llu cycle_duration_ticks: %llu\n", cpu->clock_mode, cpu->HZ_RATE, cpu->cycle_duration_ns, cpu->cycle_duration_ticks);
} */

void toggle_clock_mode(cpu_state *cpu) {
    set_clock_mode(cpu, (clock_mode_t)((cpu->clock_mode + 1) % NUM_CLOCK_MODES));
    fprintf(stdout, "Clock mode: %d HZ_RATE: %llu\n", cpu->clock_mode, cpu->HZ_RATE);
}

processor_model processor_models[NUM_PROCESSOR_TYPES] = {
    { "6502 (nmos)", cpu_6502::execute_next },
    { "65C02 (cmos)", cpu_65c02::execute_next }
};

const char* processor_get_name(int processor_type) {
    return processor_models[processor_type].name;
}

/** State storage for non-slot devices. */
void *get_module_state(cpu_state *cpu, module_id_t module_id) {
    void *state = cpu->module_store[module_id];
    if (state == nullptr) {
        fprintf(stderr, "Module %d not initialized\n", module_id);
    }
    return state;
}

void set_module_state(cpu_state *cpu, module_id_t module_id, void *state) {
    cpu->module_store[module_id] = state;
}

/** State storage for slot devices. */
SlotData *get_slot_state(cpu_state *cpu, SlotType_t slot) {
    SlotData *state = cpu->slot_store[slot];
    /* if (state == nullptr) {
        fprintf(stderr, "Slot Data for slot %d not initialized\n", slot);
    } */
    return state;
}

SlotData *get_slot_state_by_id(cpu_state *cpu, device_id id) {
    for (int i = 0; i < 8; i++) {
        if (cpu->slot_store[i] && cpu->slot_store[i]->id == id) {
            return cpu->slot_store[i];
        }
    }
    return nullptr;
}

void set_slot_state(cpu_state *cpu, SlotType_t slot, /* void */ SlotData *state) {
    state->_slot = slot;
    cpu->slot_store[slot] = state;
}

void set_slot_irq(cpu_state *cpu, uint8_t slot, bool irq) {
    if (irq) {
        cpu->irq_asserted |= (1 << slot);
    } else {
        cpu->irq_asserted &= ~(1 << slot);
    }
}

void cpu_state::init_default_memory_map() {

    for (int i = 0; i < (RAM_KB / GS2_PAGE_SIZE); i++) {
        memory->page_info[i + 0x00].type = MEM_RAM;
        memory->page_info[i + 0x00].can_read = 1;
        memory->page_info[i + 0x00].can_write = 1;
        memory->pages_read[i + 0x00] = main_ram_64 + i * GS2_PAGE_SIZE;
        memory->pages_write[i + 0x00] = main_ram_64 + i * GS2_PAGE_SIZE;
    }
    for (int i = 0; i < (IO_KB / GS2_PAGE_SIZE); i++) {
        memory->page_info[i + 0xC0].type = MEM_IO;
        memory->page_info[i + 0xC0].can_read = 1;
        memory->page_info[i + 0xC0].can_write = 1;
        memory->pages_read[i + 0xC0] = main_io_4 + i * GS2_PAGE_SIZE;
        memory->pages_write[i + 0xC0] = main_io_4 + i * GS2_PAGE_SIZE;
    }
    for (int i = 0; i < (ROM_KB / GS2_PAGE_SIZE); i++) {
        memory->page_info[i + 0xD0].type = MEM_ROM;
        memory->page_info[i + 0xD0].can_read = 1;
        memory->page_info[i + 0xD0].can_write = 0;
        memory->pages_read[i + 0xD0] = main_rom_D0 + i * GS2_PAGE_SIZE;
        memory->pages_write[i + 0xD0] = main_rom_D0 + i * GS2_PAGE_SIZE;
    }
}

void cpu_state::init_memory() {
    memory = new memory_map();
    
    main_ram_64 = new uint8_t[RAM_KB];
    main_io_4 = new uint8_t[IO_KB];
    main_rom_D0 = new uint8_t[ROM_KB];

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
    init_default_memory_map();
    #endif
}

void cpu_state::init() {
    init_memory();

    pc = 0x0400;
    sp = rand() & 0xFF; // simulate a random stack pointer
    a = 0;
    x = 0;
    y = 0;
    p = 0;
    cycles = 0;
    last_tick = 0;
    event_queue = new EventQueue();
    if (!event_queue) {
        std::cerr << "Failed to allocate event queue for CPU " << std::endl;
        exit(1);
    }
    
    trace = true;
    trace_buffer = new system_trace_buffer(100000);

    set_clock_mode(this, CLOCK_1_024MHZ);
}

void cpu_state::set_processor(int processor_type) {
    execute_next = processor_models[processor_type].execute_next;
}

void cpu_state::reset() {
    halt = 0; // if we were STPed etc.
    pc = read_word(this, RESET_VECTOR);
}