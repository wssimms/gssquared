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

    disk_t disk = { };       // start with zeroed disk.
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
