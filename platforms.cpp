#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include "cpu.hpp"
#include "platforms.hpp"

static  platform_info platforms[] = {
    { 0, "Apple II", "apple2", PROCESSOR_6502 },
    { 1, "Apple II Plus", "apple2_plus", PROCESSOR_6502 },
    { 2, "Apple IIe",     "apple2e", PROCESSOR_6502 },
    { 3, "Apple IIe Enhanced",     "apple2e_enhanced", PROCESSOR_65C02 },
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

    // Read base address
    snprintf(filepath, sizeof(filepath), "roms/%s/base.addr", platform->rom_dir);
    FILE* base_file = fopen(filepath, "r");
    if (!base_file) {
        fprintf(stderr, "Failed to open %s\n", filepath);
        delete roms;
        return nullptr;
    }
    char base_addr_str[10];
    fgets(base_addr_str, sizeof(base_addr_str), base_file);
    fclose(base_file);
    roms->main_base_addr = strtol(base_addr_str, NULL, 16);

    // Load main ROM
    snprintf(filepath, sizeof(filepath), "roms/%s/main.rom", platform->rom_dir);
    if (stat(filepath, &st) != 0) {
        fprintf(stderr, "Failed to stat %s\n", filepath);
        delete roms;
        return nullptr;
    }
    roms->main_size = st.st_size;
    FILE* main_file = fopen(filepath, "rb");
    if (!main_file) {
        fprintf(stderr, "Failed to open %s\n", filepath);
        delete roms;
        return nullptr;
    }
    roms->main_rom = new uint8_t[roms->main_size];
    fread(roms->main_rom, 1, roms->main_size, main_file);
    fclose(main_file);

    // Load character ROM
    snprintf(filepath, sizeof(filepath), "roms/%s/char.rom", platform->rom_dir);
    if (stat(filepath, &st) != 0) {
        fprintf(stderr, "Failed to stat %s\n", filepath);
        delete[] roms->main_rom;
        delete roms;
        return nullptr;
    }
    roms->char_size = st.st_size;
    FILE* char_file = fopen(filepath, "rb");
    if (!char_file) {
        fprintf(stderr, "Failed to open %s\n", filepath);
        delete[] roms->main_rom;
        delete roms;
        return nullptr;
    }
    roms->char_rom = new uint8_t[roms->char_size];
    fread(roms->char_rom, 1, roms->char_size, char_file);
    fclose(char_file);

    fprintf(stdout, "ROM Data:\n");
    fprintf(stdout, "  Main ROM Base Address: 0x%04X\n", roms->main_base_addr);
    fprintf(stdout, "  Main ROM Size: %zu bytes\n", roms->main_size);
    fprintf(stdout, "  Character ROM Size: %zu bytes\n", roms->char_size);

    return roms;
}

// Helper function to free ROM data
void free_platform_roms(rom_data* roms) {
    if (roms) {
        delete[] roms->main_rom;
        delete[] roms->char_rom;
        delete roms;
    }
}

void print_platform_info(platform_info *platform) {
    fprintf(stdout, "Platform ID %d: %s \n", platform->id, platform->name);
    fprintf(stdout, "  processor type: %s\n", processor_get_name(platform->processor_type));
    fprintf(stdout, "  folder name: %s\n", platform->rom_dir);
}
