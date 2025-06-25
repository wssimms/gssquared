#pragma once

#include <vector>
#include "Matrix3x3.hpp"

#define FRAME_WIDTH 560
#define FRAME_HEIGHT 192
#define CHEBYSHEV_SIDELOBE_DB 45

#define NUM_TAPS 7  // 5 = 11 bits (32K)  (same)
                    // 6 = 13 bits (128K) (same)
                    // 7 = 15 bits (512K) (same)
                    // 8 = 17 bits (2M)   (diff)  

// Constants
const float NTSC_FSC = 3.579545e6; // NTSC colorburst frequency
const float NTSC_4FSC = 4 * NTSC_FSC;

struct ntsc_config {
    int width = 0;
    int height = 0;
    float colorBurst = 0.0f;
    float subcarrier = 0.0f;
    float videoSaturation = 1.0f;
    float videoHue = 0.0f;
    float videoBrightness = 0.0f;

    std::vector<std::vector<float>> filterCoefficients = std::vector<std::vector<float>>(9, std::vector<float>(3, 0.0f));
    float decoderOffset[3] {0.0f, 0.0f, 0.0f}; // TODO: first value should brightness. -1 to +1.
    Matrix3x3 decoderMatrix;
    float phaseInfo[2] = {0.0f, 0.0f};

    float phase_sin[4] = {0.0f, 0.0f, 0.0f, 0.0f};
    float phase_cos[4] = {0.0f, 0.0f, 0.0f, 0.0f};

    // precalculated YUV lookup table. only two input pixel values (0 and 255)
    float pixelYUV[2][4][3] = {};
};

extern ntsc_config config;

void generate_filters(int num_taps);
void init_display_configuration();