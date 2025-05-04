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

#include "display/ntsc.hpp"
#include "display/font.hpp"
#include "display/filters.hpp"

uint8_t *frameBuffer;

void init_displayng() {
    // Create a 560x192 frame buffer
    frameBuffer = new uint8_t[560 * 192];

    buildHires40Font(MODEL_IIE, 0);
    setupConfig();
    generate_filters(NUM_TAPS);

    uint64_t start = SDL_GetTicksNS();
    init_hgr_LUT();
    uint64_t end = SDL_GetTicksNS();
    std::cout << "init_hgr_LUT took " << (end - start) / 1000.0 << " microseconds" << std::endl;
}