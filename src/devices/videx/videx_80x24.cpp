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

#include "cpu.hpp"
#include "videx.hpp"
#include "videx_80x24.hpp"

void videx_memory_write(cpu_state *cpu, SlotType_t slot, uint16_t address, uint8_t value) {
    //videx_data * videx_d = (videx_data *)get_module_state(cpu, MODULE_VIDEX);
    DisplayVidex * videx_d = (DisplayVidex *)get_slot_state(cpu, slot);
    uint16_t faddr = (videx_d->selected_page * 2) * 0x100 + (address & 0x1FF);
    videx_d->screen_memory[faddr] = value;
    videx_d->videx_set_line_dirty_by_addr(faddr);
}

uint8_t videx_memory_read(cpu_state *cpu, SlotType_t slot, uint16_t address) {
    //videx_data * videx_d = (videx_data *)get_module_state(cpu, MODULE_VIDEX);
    DisplayVidex * videx_d = (DisplayVidex *)get_slot_state(cpu, slot);
    uint16_t faddr = (videx_d->selected_page * 2) * 0x100 + (address & 0x1FF);
    return videx_d->screen_memory[faddr];
}

void videx_memory_write2(void *context, uint16_t address, uint8_t value) {
    DisplayVidex * videx_d = (DisplayVidex *)context;
    uint16_t faddr = (videx_d->selected_page * 2) * 0x100 + (address & 0x1FF);
    videx_d->screen_memory[faddr] = value;
    videx_d->videx_set_line_dirty_by_addr(faddr);
}

uint8_t videx_memory_read2(void *context, uint16_t address) {
    DisplayVidex * videx_d = (DisplayVidex *)context;
    uint16_t faddr = (videx_d->selected_page * 2) * 0x100 + (address & 0x1FF);
    return videx_d->screen_memory[faddr];
}