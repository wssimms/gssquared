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

#pragma once

#include <stdint.h>
#include <stddef.h>
#include "cpu.hpp"
#include "clock.hpp"

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