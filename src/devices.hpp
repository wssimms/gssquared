#pragma once

#include "gs2.hpp"
#include "cpu.hpp"

typedef enum device_id {
    DEVICE_ID_END = 0,
    DEVICE_ID_KEYBOARD_IIPLUS,
    DEVICE_ID_KEYBOARD_IIE,
    DEVICE_ID_SPEAKER,
    DEVICE_ID_DISPLAY,
    DEVICE_ID_GAMECONTROLLER,
    DEVICE_ID_LANGUAGE_CARD,
    DEVICE_ID_PRODOS_BLOCK,
    DEVICE_ID_PRODOS_CLOCK,
    DEVICE_ID_DISK_II,
    DEVICE_ID_MEM_EXPANSION,
    DEVICE_ID_THUNDER_CLOCK,
    NUM_DEVICE_IDS
} device_id;

struct DeviceMap_t {
    device_id id;
    uint8_t slot;
};

struct Device_t {
    device_id id;
    const char *name;
    void (*power_on)(cpu_state *cpu, uint8_t slot_number);
    void (*power_off)(cpu_state *cpu, uint8_t slot_number);
};

Device_t *get_device(device_id id);

extern Device_t NoDevice;
