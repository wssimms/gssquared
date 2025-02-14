#include <unordered_map>

#include "cpu.hpp"
#include "media.hpp"
#include "mount.hpp"
#include "devices/diskii/diskii.hpp"
#include "devices/prodos_block/prodos_block.hpp"

/**
 * Media Key
 * key = (hex) SSUU
 * S = Slot (key >> 8)
 * U = Unit (key & 0xFF)
 **/


int Mounts::mount_media(cpu_state *cpu, disk_mount_t disk_mount) {

    fprintf(stdout,"Mounting disk %s in slot %d drive %d\n", disk_mount.filename, disk_mount.slot, disk_mount.drive);
    media_descriptor * media = new media_descriptor();
    media->filename = disk_mount.filename;
    if (identify_media(*media) != 0) {
        fprintf(stderr, "Failed to identify media %s\n", disk_mount.filename);
        return false;
    }
    display_media_descriptor(*media);

    uint64_t key = (disk_mount.slot << 8) | disk_mount.drive;
    mounted_media[key].media = media;
    mounted_media[key].key = key;

    // TODO: this should look up what type of disk device is in the slot
    if (disk_mount.slot == 6) {
        mount_diskII(&CPUs[0], disk_mount.slot, disk_mount.drive, media);
        mounted_media[key].drive_type = DRIVE_TYPE_DISKII;
    } else if (disk_mount.slot == 5) {
        mount_prodos_block(disk_mount.slot, disk_mount.drive, media);
        mounted_media[key].drive_type = DRIVE_TYPE_PRODOS_BLOCK;
    } else {
        fprintf(stderr, "Invalid slot. Expected 5 or 6\n");
    }

    return key;
}

int Mounts::unmount_media(cpu_state *cpu, disk_mount_t disk_mount) {
    // TODO
    return false;
}

drive_status_t Mounts::media_status(cpu_state *cpu, uint64_t key) {
    auto it = mounted_media.find(key);
    if (it == mounted_media.end()) {
        return {false, nullptr, false, 0};
    }
    if (it->second.drive_type == DRIVE_TYPE_DISKII) {
        return diskii_status(cpu, key);
    } /* else if (it->second.drive_type == DRIVE_TYPE_PRODOS_BLOCK) {
        return prodos_block_status(cpu, key);
    } */
}


