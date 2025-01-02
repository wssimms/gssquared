#pragma once

#include <stdint.h>
#include <stddef.h>

struct platform_info {
    const char* name;           // Human readable name
    const char* rom_dir;        // Directory under roms/
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
rom_data* load_platform_roms(int index);
void free_platform_roms(rom_data* roms); 