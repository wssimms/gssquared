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

#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <time.h>
#include <getopt.h>
#include <regex>
#include <SDL3/SDL_main.h>

#include "gs2.hpp"
#include "paths.hpp"
#include "cpu.hpp"
#include "clock.hpp"
#include "display/display.hpp"
#include "opcodes.hpp"
#include "debug.hpp"
#include "test.hpp"
#include "display/text_40x24.hpp"
#include "event_poll.hpp"
#include "devices/speaker/speaker.hpp"
#include "devices/loader.hpp"
#include "platforms.hpp"
#include "util/media.hpp"
#include "util/dialog.hpp"
#include "util/mount.hpp"
#include "ui/OSD.hpp"
#include "systemconfig.hpp"
#include "slots.hpp"
#include "util/soundeffects.hpp"
#include "devices/diskii/diskii.hpp"
#include "devices/mockingboard/mb.hpp"
#include "videosystem.hpp"
#include "debugger/debugwindow.hpp"
#include "computer.hpp"
#include "mmus/mmu_ii.hpp"
#include "util/EventTimer.hpp"

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

/** Globals we haven't dealt properly with yet. */
OSD *osd = nullptr;

#if 0
cpu_state *CPUs[MAX_CPUS];
void init_cpus() { // this is the same as a power-on event.
    for (int i = 0; i < MAX_CPUS; i++) {
        CPUs[i] = new cpu_state();
        CPUs[i]->init();
    }
}
#endif

void run_cpus(computer_t *computer) {
    cpu_state *cpu = computer->cpu;

    /* initialize time tracker vars */
    uint64_t ct = SDL_GetTicksNS();
    uint64_t last_event_update = ct;
    uint64_t last_display_update = ct;
    uint64_t last_audio_update = ct;
    uint64_t last_app_event_update = ct;
    uint64_t last_5sec_update = ct;
    uint64_t last_mockingboard_update = ct;
    uint64_t last_5sec_cycles = cpu->cycles;

    uint64_t last_cycle_count =cpu->cycles;
    uint64_t last_cycle_time = SDL_GetTicksNS();

    uint64_t last_time_window_start = 0;
    uint64_t last_cycle_window_start = 0;

    while (1) {
        uint64_t cycle_window_start = cpu->cycles;
        uint64_t cycle_window_delta = cycle_window_start - last_cycle_window_start;

        uint64_t last_cycle_count = cpu->cycles;
        uint64_t last_cycle_time = SDL_GetTicksNS();

        uint64_t cycles_for_this_burst = clock_mode_info[cpu->clock_mode].cycles_per_burst;

        if (! cpu->halt) {
            switch (cpu->execution_mode) {
                    case EXEC_NORMAL:
                        while (cpu->cycles - last_cycle_count < cycles_for_this_burst) { // 1/60th second.
                            computer->event_timer->processEvents(cpu->cycles); // TODO: implement a cache to speed up this check.
                            (cpu->execute_next)(cpu);
                            if (computer->debug_window->window_open) {
                                /* if (cpu->trace_entry.eaddr == 0x03FE) {
                                    cpu->execution_mode = EXEC_STEP_INTO;
                                    cpu->instructions_left = 0;
                                    break;
                                } */
                                if (cpu->trace_entry.opcode == 0x00) { // catch a BRK and stop execution.
                                    cpu->execution_mode = EXEC_STEP_INTO;
                                    cpu->instructions_left = 0;
                                    break;
                                }
                            }
                        }
                        break;
                    case EXEC_STEP_INTO:
                        while (cpu->instructions_left) {
                            computer->event_timer->processEvents(cpu->cycles); // TODO: implement a cache to speed up this check.
                            (cpu->execute_next)(cpu);
                            cpu->instructions_left--;
                        }
                        break;
                    case EXEC_STEP_OVER:
                        break;
                }
        } else {
            // fake-increment cycle counter to keep audio in sync.
            last_cycle_count = cpu->cycles;
            cpu->cycles += cycles_for_this_burst;
        }

        uint64_t current_time;
        uint64_t audio_time;
        uint64_t display_time;
        uint64_t event_time;
        uint64_t app_event_time;

        bool this_free_run = (cpu->clock_mode == CLOCK_FREE_RUN) || (cpu->execution_mode == EXEC_STEP_INTO || (gs2_app_values.disk_accelerator && (any_diskii_motor_on(cpu))));

        if ((this_free_run) && (current_time - last_event_update > 16667000)
            || (!this_free_run)) {
            current_time = SDL_GetTicksNS();

            //computer->dispatch->processEvents();

            SDL_Event event;
            while(SDL_PollEvent(&event)) {
                // check for system "pre" events
                if (computer->sys_event->dispatch(event)) {
                    continue;
                }
                if (computer->debug_window->handle_event(event)) { // ignores event if not for debug window
                    continue;
                }
                if (!osd->event(event)) { // if osd doesn't handle it..
                    computer->dispatch->dispatch(event); // they say call "once per frame"
                }
            }

            osd->update();
            bool diskii_run = any_diskii_motor_on(cpu);
            soundeffects_update(diskii_run, diskii_tracknumber_on(cpu));

            event_time = SDL_GetTicksNS() - current_time;
            last_event_update = current_time;
        }

        /* Emit Audio Frame */
        current_time = SDL_GetTicksNS();
        if ((this_free_run) && (current_time - last_audio_update > 16667000)
            || (!this_free_run)) {            
            audio_generate_frame(cpu, last_cycle_window_start, cycle_window_start);
            audio_time = SDL_GetTicksNS() - current_time;
            last_audio_update = current_time;
        }

        current_time = SDL_GetTicksNS();
        if ((this_free_run) && (current_time - last_app_event_update > 16667000)
            || (!this_free_run)) {
            Event *event = computer->event_queue->getNextEvent();
            if (event) {
                switch (event->getEventType()) {
                    case EVENT_PLAY_SOUNDEFFECT:
                        soundeffects_play(event->getEventData());
                        break;
                    case EVENT_REFOCUS:
                        computer->video_system->raise();
                        break;
                    case EVENT_MODAL_SHOW:
                        osd->show_diskii_modal(event->getEventKey(), event->getEventData());
                        break;
                    case EVENT_MODAL_CLICK:
                        {
                            uint64_t key = event->getEventKey();
                            uint64_t data = event->getEventData();
                            printf("EVENT_MODAL_CLICK: %llu %llu\n", key, data);
                            if (data == 1) {
                                // save and unmount.
                                computer->mounts->unmount_media(key, SAVE_AND_UNMOUNT);
                                osd->event_queue->addEvent(new Event(EVENT_PLAY_SOUNDEFFECT, 0, SE_SHUGART_OPEN));
                            } else if (data == 2) {
                                // save as - need to open file dialog, get new filename, change media filename, then unmount.
                            } else if (data == 3) {
                                // discard
                                computer->mounts->unmount_media(key, DISCARD);
                                osd->event_queue->addEvent(new Event(EVENT_PLAY_SOUNDEFFECT, 0, SE_SHUGART_OPEN));
                            } else if (data == 4) {
                                // cancel
                                // Do nothing!
                            }
                            osd->close_diskii_modal(key, data);
                        }
                        break;
                    case EVENT_SHOW_MESSAGE:
                        osd->set_heads_up_message((const char *)event->getEventData(), 512);
                        break;
                 
                }
                delete event; // processed, we can now delete it.
            }
            app_event_time = SDL_GetTicksNS() - current_time;
            last_app_event_update = current_time;
        }

#if MOCKINGBOARD_ENABLED
        /* Emit Mockingboard Frame */
        current_time = SDL_GetTicksNS();
        if ((this_free_run) && (current_time - last_mockingboard_update > 16667000)
            || (!this_free_run)) {
            // TODO: need to iterate slots and call their "generate_frame" functions as appropriate.
            mb_cpu_data *mb_d = (mb_cpu_data *)get_slot_state(cpu, SLOT_4);
            if (mb_d) generate_mockingboard_frame(cpu, SLOT_4);
            last_mockingboard_update = current_time;
        }
#endif
        /* Emit Video Frame */
        current_time = SDL_GetTicksNS();
        if ((this_free_run) && (current_time - last_display_update > 16667000)
            || (!this_free_run)) {
            update_flash_state(cpu);
            update_display(cpu);    
            osd->render();
            computer->debug_window->render();
            /* display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY); */
            computer->video_system->present();
            display_time = SDL_GetTicksNS() - current_time;
            last_display_update = current_time;
        }

        /* Emit 5-second Stats */
        current_time = SDL_GetTicksNS();
        if (current_time - last_5sec_update > 5000000000) {
            uint64_t delta = cpu->cycles - last_5sec_cycles;
            cpu->e_mhz = (float)delta / float(5000000);

            fprintf(stdout, "%llu delta %llu cycles clock-mode: %d CPS: %f MHz [ slips: %llu, busy: %llu, sleep: %llu]\n", delta, cpu->cycles, cpu->clock_mode, cpu->e_mhz, cpu->clock_slip, cpu->clock_busy, cpu->clock_sleep);
            fprintf(stdout, "event_time: %10llu, audio_time: %10llu, display_time: %10llu, app_event_time: %10llu, total: %10llu\n", event_time, audio_time, display_time, app_event_time, event_time + audio_time + display_time + app_event_time);
            fprintf(stdout, "PC: %04X, A: %02X, X: %02X, Y: %02X, P: %02X\n", cpu->pc, cpu->a, cpu->x, cpu->y, cpu->p);
            last_5sec_cycles = cpu->cycles;
            last_5sec_update = current_time;
            //parallel_check_close(cpu);
        }

        if (cpu->halt == HLT_USER) {
            update_display(cpu); // update one last time to show the last state.
            break;
        }

        // calculate what sleep-until time should be.
        uint64_t wakeup_time = last_cycle_time + (cpu->cycles - last_cycle_count) * cpu->cycle_duration_ns;

        if (!this_free_run)  {
            uint64_t sleep_loops = 0;
            uint64_t current_time = SDL_GetTicksNS();
            if (current_time > wakeup_time) {
                cpu->clock_slip++;
                printf("Clock slip: event_time: %10llu, audio_time: %10llu, display_time: %10llu, app_event_time: %10llu, total: %10llu\n", event_time, audio_time, display_time, app_event_time, event_time + audio_time + display_time + app_event_time);
            } else {
                if (gs2_app_values.sleep_mode) {
                    SDL_DelayPrecise(wakeup_time - SDL_GetTicksNS());
                } else {
                    // busy wait sync cycle time
                    do {
                        sleep_loops++;
                    } while (SDL_GetTicksNS() < wakeup_time);
                }
            }
        }

        //printf("event_time / audio time / display time / total time: %9llu %9llu %9llu %9llu\n", event_time, audio_time, display_time, event_time + audio_time + display_time);

        last_cycle_count = cpu->cycles;
        last_cycle_time = SDL_GetTicksNS();

        //last_time_window_start = time_window_start;
        last_cycle_window_start = cycle_window_start;
    }
    cpu->trace_buffer->save_to_file(gs2_app_values.pref_path + "trace.bin");
}

gs2_app_t gs2_app_values;

int main(int argc, char *argv[]) {
    std::cout << "Booting GSSquared!" << std::endl;

    int platform_id = PLATFORM_APPLE_II_PLUS;  // default to Apple II Plus
    int opt;
    
    char slot_str[2], drive_str[2] /* , filename[256] */;
    int slot, drive;
    
    std::vector<disk_mount_t> disks_to_mount;

    if (isatty(fileno(stdin))) {
        gs2_app_values.console_mode = true;
    }

    gs2_app_values.base_path = get_base_path(gs2_app_values.console_mode);
    printf("base_path: %s\n", gs2_app_values.base_path.c_str());
    gs2_app_values.pref_path = get_pref_path();
    printf("pref_path: %s\n", gs2_app_values.pref_path.c_str());

    if (gs2_app_values.console_mode) {
        // parse command line optionss
        while ((opt = getopt(argc, argv, "sxp:d:")) != -1) {
            switch (opt) {
                case 'p':
                    platform_id = std::stoi(optarg);
                    break;
                case 'd':
                    {
                        std::string filename;
                        std::string arg_str(optarg);
                        // Using regex for better parsing
                        std::regex disk_pattern("s([0-9]+)d([0-9]+)=(.+)");
                        std::smatch matches;
                    
                        if (std::regex_match(arg_str, matches, disk_pattern) && matches.size() == 4) {
                            slot = std::stoi(matches[1]);
                            drive = std::stoi(matches[2]) - 1;
                            filename = matches[3];
                            //std::cout << std::format("Mounting disk {} in slot {} drive {}\n", filename, slot, drive) << std::endl;
                            std::cout << "Mounting disk " << filename << " in slot " << slot << " drive " << drive << std::endl;
                            disks_to_mount.push_back({slot, drive, filename});
                        }
                    }
                    break;
                case 'x':
                    gs2_app_values.disk_accelerator = true;
                    break;
                case 's':
                    gs2_app_values.sleep_mode = true;
                    break;
                default:
                    std::cerr << "Usage: " << argv[0] << " [-p platform] [-dsXdX=filename] [-x] [-s] \n";
                    std::cerr << "  -s: sleep mode (don't busy-wait, sleep)\n";
                    std::cerr << "  -x: disk accelerator (speed up CPU when disk II drive is active)\n";
                    exit(1);
            }
        }
    }

    // Debug print mounted media
    std::cout << "Mounted Media (" << disks_to_mount.size() << " disks):" << std::endl;
    for (const auto& disk_mount : disks_to_mount) {
        std::cout << " Slot " << disk_mount.slot << " Drive " << disk_mount.drive << " - " << disk_mount.filename << std::endl;
    }

    /* system_diag((char *)gs2_app_values.base_path); */

    computer_t *computer = new computer_t();

    //init_cpus();

// load platform roms - this info should get stored in the 'computer'
    platform_info* platform = get_platform(platform_id);
    print_platform_info(platform);

    rom_data *rd = load_platform_roms(platform);
    if (!rd) {
        system_failure("Failed to load platform roms, exiting.");
        exit(1);
    }

    // we will ALWAYS have a 256 page map. because it's a 6502 and all is addressible in a II.
    // II can have 4k, 8k, 12k; or 16k, 32k, 48k.
    // II Plus can have 16k, 32K, or 48k RAM. 16K more BUT IN THE LANGUAGE CARD MODULE.
    // always 12k rom, but not necessarily always the same ROM.
    MMU_II *mmu = new MMU_II(256, 48*1024, (uint8_t *) rd->main_rom_data);
    computer->cpu->set_mmu(mmu);
    computer->set_mmu(mmu);

    // need to tell the MMU about our ROM somehow.
    // need a function in MMU to "reset page to default".

    computer->cpu->set_processor(platform->processor_type);
    computer->mounts = new Mounts(computer->cpu); // TODO: this should happen in a CPU constructor.

    //computer->cpu->set_video_system(computer->video_system);

    computer->cpu->rd = rd;
    //init_display_font(rd);

    SystemConfig_t *system_config = get_system_config(platform_id);

    SlotManager_t *slot_manager = new SlotManager_t();

    for (int i = 0; system_config->device_map[i].id != DEVICE_ID_END; i++) {
        DeviceMap_t dm = system_config->device_map[i];

        Device_t *device = get_device(dm.id);
        device->power_on(computer, dm.slot);
        if (dm.slot != SLOT_NONE) {
            slot_manager->register_slot(device, dm.slot);
        }
    }

    bool result = soundeffects_init(computer->cpu);

    computer->cpu->reset();

    // mount disks - AFTER device init.
    while (!disks_to_mount.empty()) {
        disk_mount_t disk_mount = disks_to_mount.back();
        disks_to_mount.pop_back(); 

        computer->mounts->mount_media(disk_mount);
    }

    video_system_t *vs = computer->video_system;
    osd = new OSD(computer, computer->cpu, vs->renderer, vs->window, slot_manager, 1120, 768);
    // TODO: this should be handled differently. have osd save/restore?
    int error = SDL_SetRenderTarget(vs->renderer, nullptr);
    if (!error) {
        fprintf(stderr, "Error setting render target: %s\n", SDL_GetError());
        return 1;
    }

// the first time through (maybe the first couple times?) these will take
// considerable time. do them now before main loop.
    update_display(computer->cpu); // check for events 60 times per second.
    for (int i = 0; i < 10; i++) {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            //event_poll(computer, event); // call first time to get things started.
            computer->dispatch->dispatch(event);
        }
    }

    run_cpus(computer);

    printf("CPU halted: %d\n", computer->cpu->halt);
    if (computer->cpu->halt == HLT_INSTRUCTION) { // keep screen up and give user a chance to see the last state.
        printf("Press Enter to continue...");
        getchar();
    }

    //dump_full_speaker_event_log();

    delete computer;
    return 0;
}
