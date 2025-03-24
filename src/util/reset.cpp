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

#include "gs2.hpp"
#include "reset.hpp"
#include "cpu.hpp"
#include "devices/diskii/diskii.hpp"
#include "devices/languagecard/languagecard.hpp"
#include "devices/parallel/parallel.hpp"
#include "memory.hpp"

// TODO: implement register_reset_handler so a device can register a reset handler.
// and when system_reset is called, it will call all the registered reset handlers.

void system_reset(cpu_state *cpu, bool cold_start)  {
    // TODO: this should be a callback from the CPU reset handler.
    if (cold_start) {
        // force a cold start reset
        raw_memory_write(cpu, 0x3f2, 0x00);
        raw_memory_write(cpu, 0x3f3, 0x00);
        raw_memory_write(cpu, 0x3f4, 0x00);
    }

    cpu_reset(cpu);
    diskii_reset(cpu);
    init_default_memory_map(cpu);
    reset_languagecard(cpu); // reset language card
    parallel_reset(cpu);
}
