#pragma once

#include <stdint.h>
#include "../cpu.hpp"

void render_lores(cpu_state *cpu, int x, int y, void *pixels, int pitch);
