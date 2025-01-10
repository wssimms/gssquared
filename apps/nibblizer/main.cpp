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

int main() {

    nibblized_disk_t disk = { };       // start with zeroed disk.
    memcpy(disk.interleave_phys_to_logical, do_phys_to_logical, sizeof(interleave_t));
    memcpy(disk.interleave_logical_to_phys, do_logical_to_phys, sizeof(interleave_t));
    
    sector_t sectors[16];
    disk_image_t disk_image;

    int ret = load_disk_image(disk_image, "disk_sample.do");
    if (ret < 0) {
        printf("Failed to load disk image\n");
        exit(1);
    }
    dump_disk_image(disk_image);

    emit_disk(disk, disk_image, DEFAULT_VOLUME);
    write_disk(disk, "disk_sample.nib");

    dump_disk(disk);

}
