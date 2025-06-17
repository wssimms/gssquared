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

#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "cpu.hpp"
#include "platforms.hpp"
#include "util/ResourceFile.hpp"
#include "util/dialog.hpp"
#include "ui/MainAtlas.hpp"

static  platform_info platforms[] = {
    { 
        PLATFORM_APPLE_II, 
        "Apple II", 
        "apple2", 
        0xD000, 
        PROCESSOR_6502, 
        CLOCK_1_024MHZ 
    },
    { 
        PLATFORM_APPLE_II_PLUS, 
        "Apple II Plus", 
        "apple2_plus", 
        0xD000, 
        PROCESSOR_6502, 
        CLOCK_1_024MHZ 
        },
    { 
        PLATFORM_APPLE_IIE, 
        "Apple IIe", 
        "apple2e", 
        0xD000, 
        PROCESSOR_6502, 
        CLOCK_1_024MHZ 
    },
    { 
        PLATFORM_APPLE_IIE_ENHANCED, 
        "Apple IIe Enhanced",   
        "apple2e_enhanced", 
        0xD000, 
        PROCESSOR_65C02, 
        CLOCK_1_024MHZ
     },
    // Add more platforms as needed:
    // { "Apple IIc",         "apple2c" },
    // { "Apple IIc Plus",    "apple2c_plus" },
};

int num_platforms = sizeof(platforms) / sizeof(platforms[0]);

// Helper function to get platform info by index
 platform_info* get_platform(int index) {
    if (index >= 0 && index < num_platforms) {
        return &platforms[index];
    }
    return nullptr;
}

// Helper function to find platform by rom directory name
 platform_info* find_platform_by_dir(const char* dir) {
    for (int i = 0; i < num_platforms; i++) {
        if (strcmp(platforms[i].rom_dir, dir) == 0) {
            return &platforms[i];
        }
    }
    return nullptr;
}

rom_data* load_platform_roms(platform_info *platform) {
    if (!platform) return nullptr;

    fprintf(stderr, "Platform: %s   folder name: %s\n", platform->name, platform->rom_dir);

    rom_data* roms = new rom_data();
    char filepath[256];
    struct stat st;

 /*    // Read base address
    snprintf(filepath, sizeof(filepath), "roms/%s/base.addr", platform->rom_dir);
    FILE* base_file = fopen(filepath, "r");
    if (!base_file) {
        char *debugstr = new char[512];
        snprintf(debugstr, 512, "Failed to open base_addr %s errno: %d\n", filepath, errno);
        system_failure(debugstr);
        delete roms;
        return nullptr;
    }
    char base_addr_str[10];
    fgets(base_addr_str, sizeof(base_addr_str), base_file);
    fclose(base_file);
    roms->main_base_addr = strtol(base_addr_str, NULL, 16);
 */

    roms->main_base_addr = platform->rom_base_addr;

    // Load main ROM
    snprintf(filepath, sizeof(filepath), "roms/%s/main.rom", platform->rom_dir);
    roms->main_rom_file = new ResourceFile(filepath, READ_ONLY);
    if (!roms->main_rom_file->exists()) {
        char *debugstr = new char[512];
        snprintf(debugstr, 512, "Failed to stat %s errno: %d\n", filepath, errno);
        system_failure(debugstr);
        delete roms;
        return nullptr;
    }
    roms->main_rom_data = (main_rom_t*) roms->main_rom_file->load();
    //roms->main_rom_file->size();

    // Load character ROM
    snprintf(filepath, sizeof(filepath), "roms/%s/char.rom", platform->rom_dir);
    roms->char_rom_file = new ResourceFile(filepath, READ_ONLY);
    if (!roms->char_rom_file->exists()) {
        char *debugstr = new char[512];
        snprintf(debugstr, 512, "Failed to stat %s errno: %d\n", filepath, errno);
        system_failure(debugstr);
        delete[] roms->main_rom_file;
        delete roms;
        return nullptr;
    }
    roms->char_rom_data = (char_rom_t*) roms->char_rom_file->load();
    //roms->char_size = char_rom_file.size();
    roms->char_rom_file->dump();

    fprintf(stdout, "ROM Data:\n");
    fprintf(stdout, "  Main ROM Base Address: 0x%04X\n", roms->main_base_addr);
    fprintf(stdout, "  Main ROM Size: %zu bytes\n", roms->main_rom_file->size());
    fprintf(stdout, "  Character ROM Size: %zu bytes\n", roms->char_rom_file->size());

    return roms;
}

// Helper function to free ROM data
void free_platform_roms(rom_data* roms) {
    if (roms) {
        delete roms->main_rom_file;
        delete roms->char_rom_file;
        delete roms;
    }
}

void print_platform_info(platform_info *platform) {
    fprintf(stdout, "Platform ID %d: %s \n", platform->id, platform->name);
    fprintf(stdout, "  processor type: %s\n", processor_get_name(platform->processor_type));
    fprintf(stdout, "  folder name: %s\n", platform->rom_dir);
}
