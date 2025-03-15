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

#include "util/media.hpp"
#include "util/mount.hpp"
#define DiskII_Ph0_Off 0x00
#define DiskII_Ph0_On 0x01
#define DiskII_Ph1_Off 0x02
#define DiskII_Ph1_On 0x03
#define DiskII_Ph2_Off 0x04
#define DiskII_Ph2_On 0x05
#define DiskII_Ph3_Off 0x06
#define DiskII_Ph3_On 0x07
#define DiskII_Motor_Off 0x08
#define DiskII_Motor_On 0x09
#define DiskII_Drive1_Select 0x0A
#define DiskII_Drive2_Select 0x0B
#define DiskII_Q6L 0x0C
#define DiskII_Q6H 0x0D
#define DiskII_Q7L 0x0E
#define DiskII_Q7H 0x0F

void init_slot_diskII(cpu_state *cpu, SlotType_t slot);
void mount_diskII(cpu_state *cpu, uint8_t slot, uint8_t drive, media_descriptor *media);
void unmount_diskII(cpu_state *cpu, uint8_t slot, uint8_t drive);
drive_status_t diskii_status(cpu_state *cpu, uint64_t key);
void diskii_reset(cpu_state *cpu);
