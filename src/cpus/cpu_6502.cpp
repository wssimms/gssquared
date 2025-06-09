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

#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <sstream>
#include <iomanip>
#include <time.h>
/* #include <mach/mach_time.h> */
#include <getopt.h>

#include "gs2.hpp"
#include "cpu.hpp"
#include "clock.hpp"
#include "opcodes.hpp"
#include "debug.hpp"

#define CPU_6502

// 6502-specific wrapper

#include "core_6502.hpp"

namespace cpu_6502 {

#include "core_6502.cpp"

}
