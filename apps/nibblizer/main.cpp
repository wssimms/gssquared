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

#include <unistd.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "devices/diskii/diskii_fmt.hpp"

/**
 * for now, read only!
 */

void print_usage(const char* program_name) {
    fprintf(stderr, "Usage: %s [-v] [-o output.nib] input_file\n", program_name);
    fprintf(stderr, "  -o filename    Write output to filename (default: output.nib)\n");
    fprintf(stderr, "  -v            Verbose mode - dump disk information\n");
    exit(1);
}

int main(int argc, char *argv[]) {
    const char* output_filename = "output.nib";
    const char* input_filename = nullptr;
    bool verbose = false;
    int opt;

    // Process command line options
    while ((opt = getopt(argc, argv, "o:v")) != -1) {
        switch (opt) {
            case 'o':
                output_filename = optarg;
                break;
            case 'v':
                verbose = true;
                break;
            default:
                print_usage(argv[0]);
        }
    }

    // Get input filename (first non-option argument)
    if (optind >= argc) {
        print_usage(argv[0]);
    }
    input_filename = argv[optind];

    nibblized_disk_t disk = { };       // start with zeroed disk.
    memcpy(disk.interleave_phys_to_logical, do_phys_to_logical, sizeof(interleave_t));
    memcpy(disk.interleave_logical_to_phys, do_logical_to_phys, sizeof(interleave_t));
    
    sector_t sectors[16];
    disk_image_t disk_image;

    int ret = load_disk_image(disk_image, input_filename);
    if (ret < 0) {
        fprintf(stderr, "Failed to load disk image: %s\n", input_filename);
        exit(1);
    }
    if (verbose) {
        dump_disk_image(disk_image);
    }

    emit_disk(disk, disk_image, DEFAULT_VOLUME);
    write_disk(disk, output_filename);

    if (verbose) {
        dump_disk(disk);
    }
    return 0;
}
