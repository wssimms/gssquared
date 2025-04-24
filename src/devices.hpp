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
    DEVICE_ID_PD_BLOCK2,
    DEVICE_ID_PARALLEL,
    DEVICE_ID_VIDEX,
    DEVICE_ID_MOCKINGBOARD,
    NUM_DEVICE_IDS
} device_id;

struct DeviceMap_t {
    device_id id;
    SlotType_t slot;
};

struct Device_t {
    device_id id;
    const char *name;
    void (*power_on)(cpu_state *cpu, SlotType_t slot_number);
    void (*power_off)(cpu_state *cpu, SlotType_t slot_number);
};

Device_t *get_device(device_id id);

extern Device_t NoDevice;
