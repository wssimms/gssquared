#pragma once

#include <unordered_map>

#include "cpu.hpp"
#include "media.hpp"

typedef struct {
    int slot;
    int drive;
    char *filename;
    media_descriptor *media;
} disk_mount_t;

struct drive_status_t {
    bool is_mounted;
    char *filename;
    bool motor_on;
    int position;
};


enum drive_type_t {
    DRIVE_TYPE_DISKII,
    DRIVE_TYPE_PRODOS_BLOCK,
};

struct drive_media_t {
    uint64_t key;
    drive_type_t drive_type;
    media_descriptor *media;
};

class Mounts {
protected:
    std::unordered_map<uint64_t, drive_media_t> mounted_media;

public:

    int mount_media(cpu_state *cpu, disk_mount_t disk_mount);
    int unmount_media(cpu_state *cpu, disk_mount_t disk_mount);
    drive_status_t media_status(cpu_state *cpu, uint64_t key);
};


