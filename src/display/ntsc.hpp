#pragma once

#include <vector>
#include "Matrix3x3.hpp"
#include "display/types.hpp"
// Define types

struct ntsc_config {
    int width;
    int height;
    float colorBurst;
    float subcarrier;
    std::vector<std::vector<float>> filterCoefficients;
    float decoderOffset[3]; // TODO: first value should brightness. -1 to +1.
    Matrix3x3 decoderMatrix;
    float phaseInfo[2];
};

#define FRAME_WIDTH 560
#define FRAME_HEIGHT 192
#define NUM_TAPS 8  //  5 = 11 bits (32K) (same)  6 = 13 bits (128K) (same)  7 = 15 bits (512K) (same)  8 = 17 bits (2M) (diff)  

// Constants
const float NTSC_FSC = 3.579545e6; // NTSC colorburst frequency
const float NTSC_4FSC = 4 * NTSC_FSC;

extern ntsc_config config ;

void setupConfig();
void init_hgr_LUT();
void processAppleIIFrame_LUT(uint8_t* frameData, RGBA* outputImage, int y_start, int y_end);