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
#include "devices/keyboard/keyboard.hpp"
#include "devices/speaker.hpp"
#include "devices/loader.hpp"
#include "devices/thunderclock_plus/thunderclockplus.hpp"
#include "devices/diskii.hpp"
#include "devices/diskii/diskii_fmt.hpp"
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

        //CPUs[i].execute_next = &execute_next_6502;

        set_clock_mode(&CPUs[i], CLOCK_1_024MHZ);

        CPUs[i].next_tick = mach_absolute_time() + CPUs[i].cycle_duration_ticks; 
    }
}

void set_cpu_processor(cpu_state *cpu, int processor_type) {
    cpu->execute_next = processor_models[processor_type].execute_next;
}

void run_cpus(void) {
    cpu_state *cpu = &CPUs[0];

    uint64_t last_display_update = 0;
    uint64_t last_5sec_update = 0;
    uint64_t last_5sec_cycles;

    while (1) {
        if ((cpu->execute_next)(cpu) > 0) {
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

    disk_image_t disk_image;
    disk_t disk;
    
    while ((opt = getopt(argc, argv, "p:a:b:1:2:")) != -1) {
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
            case '1':
                mount_disk(6, 0, optarg);
                break;
            case '2':
                mount_disk(6, 1, optarg);
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
        platform_info* platform = get_platform(platform_id);
        print_platform_info(platform);

        rom_data *rd = load_platform_roms(platform);

        if (!rd) {
            fprintf(stdout, "Failed to load platform roms, exiting. Did you 'cd roms; make' first?\n");
            exit(1);
        }
        // Load into memory at correct address
        for (uint64_t i = 0; i < rd->main_size; i++) {
            raw_memory_write(&CPUs[0], rd->main_base_addr + i, rd->main_rom[i]);
        }
        // we could dispose of this now if we wanted..
#endif

    set_cpu_processor(&CPUs[0], platform->processor_type);

    if (init_display_sdl(rd)) {
        fprintf(stderr, "Error initializing display\n");
        exit(1);
    }

    init_keyboard();
    init_device_display();
    init_speaker(&CPUs[0]);
    init_thunderclock(1);
    diskII_register_slot(&CPUs[0], 6); // put a disk II in slot 6

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
