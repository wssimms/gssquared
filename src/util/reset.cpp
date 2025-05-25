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
#include "devices/mockingboard/mb.hpp"

// TODO: implement register_reset_handler so a device can register a reset handler.
// and when system_reset is called, it will call all the registered reset handlers.

// TODO: this is a higher level concept than just resetting the CPU.
void system_reset(cpu_state *cpu, bool cold_start)  {
    if (cold_start) {
        // force a cold start reset
        raw_memory_write(cpu, 0x3f2, 0x00);
        raw_memory_write(cpu, 0x3f3, 0x00);
        raw_memory_write(cpu, 0x3f4, 0x00);
    }

    cpu->reset();
    diskii_reset(cpu);
    cpu->init_default_memory_map();
    reset_languagecard(cpu); // reset language card
    parallel_reset(cpu);
#if MOCKINGBOARD_ENABLED
    mb_reset(cpu);
#endif
}
