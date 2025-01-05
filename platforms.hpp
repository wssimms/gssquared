#pragma once

#include <stdint.h>
#include <stddef.h>

struct platform_info {
    const int id;               // Human readable name
    const char* name;           // Human readable name
    const char* rom_dir;        // Directory under roms/
    const int processor_type;   // processor type
    const clock_mode default_clock_mode; // default clock mode for this platform at startup.
};

struct rom_data {
    uint16_t main_base_addr;
    uint8_t* main_rom;
    size_t main_size;
    uint8_t* char_rom;
    size_t char_size;
};

extern  int num_platforms;
platform_info* get_platform(int index);
platform_info* find_platform_by_dir(const char* dir);
rom_data* load_platform_roms(platform_info *platform);
void free_platform_roms(rom_data* roms); 
void print_platform_info(platform_info *platform);