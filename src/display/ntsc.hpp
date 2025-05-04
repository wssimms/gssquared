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

#include <vector>
#include "Matrix3x3.hpp"
#include "display.hpp"
#include "display/types.hpp"

// Define types


struct ntsc_config {
    int width;
    int height;
    float colorBurst;
    float subcarrier;
    float videoSaturation;
    float videoHue;
    float videoBrightness;

    std::vector<std::vector<float>> filterCoefficients;
    float decoderOffset[3]; // TODO: first value should brightness. -1 to +1.
    Matrix3x3 decoderMatrix;
    float phaseInfo[2];

    float phase_sin[4];
    float phase_cos[4];

    // precalculated YUV lookup table. only two input pixel values (0 and 255)
    float pixelYUV[2][4][3];
};

#define FRAME_WIDTH 560
#define FRAME_HEIGHT 192
#define NUM_TAPS 7  //  5 = 11 bits (32K) (same)  6 = 13 bits (128K) (same)  7 = 15 bits (512K) (same)  8 = 17 bits (2M) (diff)  
#define CHEBYSHEV_SIDELOBE_DB 45

// Constants
const float NTSC_FSC = 3.579545e6; // NTSC colorburst frequency
const float NTSC_4FSC = 4 * NTSC_FSC;

extern ntsc_config config ;

extern RGBA mono_color_table[DM_NUM_MONO_MODES];

void setupConfig();
void init_hgr_LUT();
void processAppleIIFrame_LUT(uint8_t* frameData, RGBA* outputImage, int y_start, int y_end);
void processAppleIIFrame_Mono(uint8_t* frameData, RGBA* outputImage, int y_start, int y_end, RGBA color_value);