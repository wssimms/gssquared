#pragma once

#include <vector>
#include <cstdint>
#include "gs2.hpp"
#include "cpu.hpp"

#define CHAR_NUM 256
#define CHAR_WIDTH 16
#define CELL_WIDTH 14

typedef enum {
    MODEL_II,
    MODEL_IIJPLUS,
    MODEL_IIE
} Model;

void render_hgrng_scanline(cpu_state *cpu, int y, uint8_t *pixels);
void emitBitSignalHGR(uint8_t *hiresData, uint8_t *bitSignalOut, size_t outputOffset, int y);
