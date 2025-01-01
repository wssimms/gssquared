
#include "../gs2.hpp"
#include "../cpu.hpp"
#include "../memory.hpp"
#include "text_40x24.hpp"
#include "lores_40x48.hpp"

uint32_t lores_color_table[16] = {
    0x00000000,
    0x8A2140FF,
    0x3C22A5FF,
    0xC847E4FF,
    0x07653EFF,
    0x7B7E80FF,
    0x308EF3FF,
    0xB9A9FDFF,
    0x3B5107FF,
    0xC77028FF,
    0x7B7E80FF,
    0xF39AC2FF,
    0x2FB81FFF,
    0xB9D060FF,
    0x6EE1C0FF,
    0xFFFFFFFF
    };


void render_lores(cpu_state *cpu, int x, int y, void *pixels, int pitch) {
    // Bounds checking
    if (x < 0 || x >= 40 || y < 0 || y >= 24) {
        return;
    }
    uint8_t character = raw_memory_read(cpu, TEXT_PAGE_TABLE[y] + x);

    // look up color key for top and bottom block
    uint32_t color_top = lores_color_table[character & 0x0F];
    uint32_t color_bottom = lores_color_table[(character & 0xF0) >> 4];

    int pitchoff = pitch / 4;
    int charoff = x * 7;
    // Draw the character bitmap into the texture
    uint32_t* texturePixels = (uint32_t*)pixels;
    for (int row = 0; row < 4; row++) {
        uint32_t base = row * pitchoff;
        texturePixels[base + charoff ] = color_top;
        texturePixels[base + charoff + 1] = color_top;
        texturePixels[base + charoff + 2] = color_top;
        texturePixels[base + charoff + 3] = color_top;
        texturePixels[base + charoff + 4] = color_top;
        texturePixels[base + charoff + 5] = color_top;
        texturePixels[base + charoff + 6] = color_top;
    }

    for (int row = 4; row < 8; row++) {
        uint32_t base = row * pitchoff;
        texturePixels[base + charoff ] = color_bottom;
        texturePixels[base + charoff + 1] = color_bottom;
        texturePixels[base + charoff + 2] = color_bottom;
        texturePixels[base + charoff + 3] = color_bottom;
        texturePixels[base + charoff + 4] = color_bottom;
        texturePixels[base + charoff + 5] = color_bottom;
        texturePixels[base + charoff + 6] = color_bottom;

    }    
}