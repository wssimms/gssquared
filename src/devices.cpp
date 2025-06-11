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

#include "devices.hpp"

#include "devices/keyboard/keyboard.hpp"
#include "devices/speaker/speaker.hpp"
#include "display/display.hpp"
#include "devices/game/gamecontroller.hpp"
#include "devices/languagecard/languagecard.hpp"
#include "devices/prodos_clock/prodos_clock.hpp"
#include "devices/diskii/diskii.hpp"
#include "devices/memoryexpansion/memexp.hpp"
#include "devices/thunderclock_plus/thunderclockplus.hpp"
#include "devices/pdblock2/pdblock2.hpp"
#include "devices/parallel/parallel.hpp"
#include "devices/videx/videx.hpp"
#include "devices/mockingboard/mb.hpp"
#include "devices/annunciator/annunciator.hpp"

Device_t NoDevice = {
        DEVICE_ID_END,
        "No Device",
        NULL,
        NULL
    };

Device_t Devices[NUM_DEVICE_IDS] = {
    {
        DEVICE_ID_KEYBOARD_IIPLUS,
        "Apple II Plus Keyboard",
        init_mb_iiplus_keyboard,
        NULL
    },
    {
        DEVICE_ID_KEYBOARD_IIE,
        "Apple IIe Keyboard",
        NULL /* init_mb_keyboard, */,
        NULL
    },
    {
        DEVICE_ID_SPEAKER,
        "Speaker",
        init_mb_speaker,
        NULL
    },
    {
        DEVICE_ID_DISPLAY,
        "Display",
        init_mb_device_display,
        NULL
    },
    {
        DEVICE_ID_GAMECONTROLLER,
        "Game Controller",
        init_mb_game_controller,
        NULL
    },
    {
        DEVICE_ID_LANGUAGE_CARD,
        "II/II+ Language Card",
        init_slot_languagecard,
        NULL
    },
    {
        DEVICE_ID_PRODOS_BLOCK,
        "Generic ProDOS Block",
        NULL, //init_prodos_block,
        NULL
    },
    {
        DEVICE_ID_PRODOS_CLOCK,
        "Generic ProDOS Clock",
        init_slot_prodosclock,
        NULL
    },
    {
        DEVICE_ID_DISK_II,
        "Disk II Controller",
        init_slot_diskII,
        NULL
    },
    {
        DEVICE_ID_MEM_EXPANSION,
        "Memory Expansion (Slinky)",
        init_slot_memexp,
        NULL
    },
    {
        DEVICE_ID_THUNDER_CLOCK,
        "Thunder Clock Plus",
        init_slot_thunderclock,
        NULL
    },
    {
        DEVICE_ID_PD_BLOCK2,
        "Generic ProDOS Block 2",
        init_pdblock2,
        NULL
    },
    {
        DEVICE_ID_PARALLEL,
        "Apple II Parallel Interface",
        init_slot_parallel,
        NULL
    },
    {
        DEVICE_ID_VIDEX,
        "Videx VideoTerm",
        init_slot_videx,
        NULL
    },
    {
        DEVICE_ID_MOCKINGBOARD,
        "Mockingboard",
        init_slot_mockingboard,
        NULL
    },
    {
        DEVICE_ID_ANNUNCIATOR,
        "Annunciator",
        init_annunciator,
        NULL
    }
};

Device_t *get_device(device_id id) {
    return &Devices[id-1];
}
