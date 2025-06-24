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

#define _USE_MATH_DEFINES // for C++ M_PI
// https://devblogs.microsoft.com/cppblog/standards-version-switches-in-the-compiler/
//  Properties - C/C++ - Launguage - C++ Language Standard - ISO C++20 Standard (/std:c++20)

#include <vector>
#include <cmath>
#include <cstdint>
#include <algorithm>
//#include <chrono>
#include <stdio.h>
#include "display.hpp"
#include "Matrix3x3.hpp"
#include "display/ntsc.hpp"
#include "devices/displaypp/RGBA.hpp"

ntsc_config config ;

// Function to generate phase information for a scanline and stuff in config.
void generatePhaseInfo(int scanlineY, float colorBurst) {
    //ph_info phaseInfo;
    
    // Normalize the color burst phase (similar to the original code)
    float c = colorBurst / (2 * M_PI);
    config.phaseInfo[0] = c - std::floor(c);
    
    // Set phase alternation (in original code this is a boolean array, 
    // but we simplify to alternate based on scanline)
    config.phaseInfo[1] = 0.0f; //(scanlineY % 2) ? 1.0f : -0.0f;

}

// The core pixel function - convert an input luminance to YIQ-like representation
inline void processPixel(uint8_t input, float phase, float phaseAlternation, float* output) {
    // Normalize input byte (0-255 luma) to 0-1 luma.
    float luminance = input / 255.0f;
#if 0    
    // Apply YIQ encoding
    output[0] = luminance;                                    // Y component
    output[1] = luminance * std::sin(phase);                  // I component
    output[2] = luminance * (1.0f - 2.0f * phaseAlternation) * std::cos(phase); // Q component
#else
    output[0] = luminance;
    output[1] = luminance * std::sin(phase);
    output[2] = luminance * std::cos(phase);
#endif
}

// Function to convert YIQ to RGB using a decoding matrix
inline void yiqToRgb(const float yiq[3], /* const Matrix3x3& matrix,  */ /* const float offset[3], */ RGBA_t &rgba) {
    float rgb[3];
    
    // Apply matrix and offset (matrix is 3x3, stored in row-major format)
    rgb[0] = config.decoderMatrix[0] * yiq[0] + config.decoderMatrix[1] * yiq[1] + config.decoderMatrix[2] * yiq[2] /* + offset[0] */;
    rgb[1] = config.decoderMatrix[3] * yiq[0] + config.decoderMatrix[4] * yiq[1] + config.decoderMatrix[5] * yiq[2] /* + offset[1] */;
    rgb[2] = config.decoderMatrix[6] * yiq[0] + config.decoderMatrix[7] * yiq[1] + config.decoderMatrix[8] * yiq[2] /* + offset[2] */;
    
    // Clamp values and convert to 8-bit
    rgba.r = std::clamp(static_cast<int>(rgb[0] * 255), 0, 255);
    rgba.g = std::clamp(static_cast<int>(rgb[1] * 255), 0, 255);
    rgba.b = std::clamp(static_cast<int>(rgb[2] * 255), 0, 255);
    rgba.a = 255;
}

void setupConfig() {
    // TODO: analyze and document - in apple ii land the colorburst is defined as negative 33 degrees,
    // But that shifts the colorburst the wrong way. We needed to bring it counterclockwise around the 
    // colorwheel.
    //float colorBurst = 2.0 * M_PI * (-33.0 / 360.0 + (imageLeft % 4) / 4.0);
    float colorBurst = (2.0 * M_PI * (33.0 / 360.0 )); // plus 0.25, 0.5 0.75, if imageLeft % 4 is 1 2 3
    float subcarrier = NTSC_FSC / NTSC_4FSC; /* 0.25, basically constant here */
    printf("ColorBurst: %.6f\n", colorBurst);
    printf("Subcarrier: %.6f\n", subcarrier);

    //config.phaseInfo = generatePhaseInfo(/* scanlineY */ 0, colorBurst);
    printf("Phase Info:\n");
    printf("Phase[0]: %.6f\n", config.phaseInfo[0]);
    printf("Phase[1]: %.6f\n", config.phaseInfo[1]);

    config.width = FRAME_WIDTH;
    config.height = FRAME_HEIGHT;
    config.colorBurst = colorBurst;
    config.subcarrier = subcarrier;
    config.videoSaturation = 1.0f;
    config.videoHue = 0.05f;
    config.videoBrightness = 0.0f;

   /* config = {
        .width = FRAME_WIDTH,
        .height = FRAME_HEIGHT,
        .colorBurst = colorBurst,
        .subcarrier = subcarrier,
        .videoSaturation = 1.0f,
        .videoHue = 0.05f,
        .videoBrightness = 0.0f,
            // Set up filter coefficients based on measured YIQ filter response
        .filterCoefficients = {
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f }
        },
        .decoderOffset = {0.0f, 0.0f, 0.0f}, // TODO: first value should brightness. -1 to +1.
        .videoSaturation = 1.0f,
        .videoHue = 0.05f,
        .videoBrightness = 0.0f
    }; */
    generatePhaseInfo(/* scanlineY */ 0, colorBurst);

    // pre-calculate the sin/cos values for each of the 4 pixel positions (phases) based on
    // phaseInfo[0] and subcarrier.
    for (int ph = 0; ph < 4; ph++) {
        config.phase_sin[ph] = sinf(2.0f * M_PI * ((config.subcarrier * (ph % 4)) + config.phaseInfo[0]));
        config.phase_cos[ph] = cosf(2.0f * M_PI * ((config.subcarrier * (ph % 4)) + config.phaseInfo[0]));
        printf("phase_sin[%d]: %.6f, phase_cos[%d]: %.6f\n", ph, config.phase_sin[ph], ph, config.phase_cos[ph]);
    }

    // 4 phases
    // pixelYUV[2][4][3]
    // 2 input pixel values (0, 1)
    // 4 phases (0, 1, 2, 3)
    // 3 YUV components (Y, U, V)

    for (int ph = 0; ph < 4; ph++) {
        config.pixelYUV[0][ph][0] = 0.0f;
        config.pixelYUV[0][ph][1] = 0.0f;
        config.pixelYUV[0][ph][2] = 0.0f;

        config.pixelYUV[1][ph][0] = 1.0f;
        config.pixelYUV[1][ph][1] = config.phase_sin[ph];
        config.pixelYUV[1][ph][2] = config.phase_cos[ph];
        printf("pixelYUV[1][%d][0]: %.6f, pixelYUV[1][%d][1]: %.6f, pixelYUV[1][%d][2]: %.6f\n", ph, config.pixelYUV[1][ph][0], ph, config.pixelYUV[1][ph][1], ph, config.pixelYUV[1][ph][2]);
        printf("pixelYUV[0][%d][0]: %.6f, pixelYUV[0][%d][1]: %.6f, pixelYUV[0][%d][2]: %.6f\n", ph, config.pixelYUV[0][ph][0], ph, config.pixelYUV[0][ph][1], ph, config.pixelYUV[0][ph][2]);
    }
}

/** 
 * the Lookup table - 4 phases, 2 ^ (num_taps * 2) bits.
 */
alignas(64) RGBA_t g_hgr_LUT[4][(1 << ((NUM_TAPS * 2) + 1))];


// Process a single scanline of Apple II video data
RGBA_t  processAppleIIScanline_lut3(
    uint32_t inputBits,         // Input width bytes of luminance data
    int pixelposition
) 
{
    // First pass: Convert input to YIQ representation
    //float *yiq = &yiqBuffer[(pixelposition - NUM_TAPS) * 3];

    float oy = 0.0f;
    float oi = 0.0f;
    float oq = 0.0f;

    //for (int x = pixelposition - NUM_TAPS; x <= pixelposition + NUM_TAPS; x++) 
    for (int offset = - NUM_TAPS; offset <= NUM_TAPS; offset++)
    {
        int x = pixelposition + offset;
        int coeffIdx = std::abs(offset);
        bool bit = (inputBits & 1); // this might be backwards.
        float y = config.pixelYUV[bit][x % 4][0];
        float i = config.pixelYUV[bit][x % 4][1];
        float q = config.pixelYUV[bit][x % 4][2];

        oy += y * config.filterCoefficients[coeffIdx][0];  // Y
        oi += i * config.filterCoefficients[coeffIdx][1];  // I
        oq += q * config.filterCoefficients[coeffIdx][2];  // Q

        inputBits >>= 1; // this might be backwards.
    }
 
    // Convert pixel to RGB
    // Apply matrix and offset (matrix is 3x3, stored in row-major format)
    float r = config.decoderMatrix[0] * oy + config.decoderMatrix[1] * oi + config.decoderMatrix[2] * oq /* + offset[0] */;
    float g = config.decoderMatrix[3] * oy + config.decoderMatrix[4] * oi + config.decoderMatrix[5] * oq /* + offset[1] */;
    float b = config.decoderMatrix[6] * oy + config.decoderMatrix[7] * oi + config.decoderMatrix[8] * oq /* + offset[2] */;
    
    RGBA_t emit;
    // Clamp values and convert to 8-bit
    emit.r = std::clamp(static_cast<int>(r * 255), 0, 255);
    emit.g = std::clamp(static_cast<int>(g * 255), 0, 255);
    emit.b = std::clamp(static_cast<int>(b * 255), 0, 255);
    emit.a = 255;

    return emit;
}

/** 
 * extract the following into variables in the ntsc_config:
 * videoSaturation: 1.0f
 * videoHue: 0.05f
 * videoBrightness: 0.0f
 * whenever you change these values, re-call init_hgr_LUT().
 */
void init_hgr_LUT()
{
    // identity matrix?
    Matrix3x3 decoderMatrix(
        1, 0, 0,
        0, 1, 0,
        0, 0, 1);

    Matrix3x3 yiqMatrix(
        1, 0, 1.13983,
        1, -0.39465, -0.58060,
        1, 2.03206, 0
    );

    // saturation should be 0 to 1.0 
    // 0 is basically monochrome. transposing row/col here does not matter.
    //float videoSaturation = 1.0f;
    Matrix3x3 saturationMatrix(
        1, 0, 0,
        0, config.videoSaturation, 0,
        0, 0, config.videoSaturation
    );
    // if hue is 0, then hueMatrix becomes identity matrix. (transposing r
    float videoHue = 2 * (float)M_PI * config.videoHue;
    Matrix3x3 hueMatrix(
        1, 0, 0,
        0, cosf(videoHue), -sinf(videoHue),
        0, sinf(videoHue), cosf(videoHue)
    );

    decoderMatrix.multiply(saturationMatrix);
    decoderMatrix.multiply(hueMatrix);
    decoderMatrix.multiply(yiqMatrix);
    decoderMatrix.print();
    config.decoderMatrix = decoderMatrix;

    for (int phaseCount = 0; phaseCount < 4; phaseCount++)
    {
        for (uint32_t bitCount = 0; bitCount < (1 << ((NUM_TAPS * 2) + 1)); bitCount++)
        {
            //  set the bit pattern on either size of the center (16 + phaseCount)

            int center = (16 + phaseCount);

            RGBA_t rval = processAppleIIScanline_lut3(bitCount,/*  outputImage, */ center /* , width, yiqBuffer, filteredYiq */);
            g_hgr_LUT[phaseCount][bitCount] = rval;  //  Pull out the center pixel and save it.
        }
    }
}

RGBA_t standard_iigs_color_table[16] = {
    { .a=0xFF, .b=0x00, .g=0x00, .r=0x00 },
    { .a=0xFF, .b=0x30, .g=0x00, .r=0xD0 },
    { .a=0xFF, .b=0x90, .g=0x00, .r=0x00 },
    { .a=0xFF, .b=0xD0, .g=0x20, .r=0xD0 },
    { .a=0xFF, .b=0x20, .g=0x70, .r=0x00 },
    { .a=0xFF, .b=0x50, .g=0x50, .r=0x50 },
    { .a=0xFF, .b=0xF0, .g=0x20, .r=0x20 },
    { .a=0xFF, .b=0xF0, .g=0xA0, .r=0x60 },
    { .a=0xFF, .b=0x00, .g=0x50, .r=0x80 },
    { .a=0xFF, .b=0x00, .g=0x60, .r=0xF0 },
    { .a=0xFF, .b=0xA0, .g=0xA0, .r=0xA0 },
    { .a=0xFF, .b=0x80, .g=0x90, .r=0xF0 },
    { .a=0xFF, .b=0x00, .g=0xD0, .r=0x10 },
    { .a=0xFF, .b=0x00, .g=0xF0, .r=0xF0 },
    { .a=0xFF, .b=0x90, .g=0xF0, .r=0x40 },
    { .a=0xFF, .b=0xF0, .g=0xF0, .r=0xF0 }
};

void newProcessAppleIIFrame_NTSC (
    cpu_state *cpu,             // access to display_state_t
    RGBA_t* outputImage           // Will be filled with 560x192 RGBA pixels
)
{
    VideoScannerII *vs = cpu->get_video_scanner();
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    ds->kill_color = true;

    uint8_t  video_idx;
    uint16_t video_bits;
    uint16_t rawbits = 0;
    uint32_t ntscidx = 0;
    uint32_t vcount = 0;
    uint32_t hcount = 0;
    uint32_t phase = 0;
    uint32_t count = 0;

    uint8_t * video_data = vs->get_video_data();
    int video_data_size = vs->get_video_data_size();
    int i = 0;

    while (i < video_data_size)
    {
        // This section builds a 14/15 bit wide video_bits for each byte of
        // video memory, based on the video_mode associated with each byte

        video_mode_t video_mode = (video_mode_t)(video_data[i++]);
        uint8_t video_byte = video_data[i++];

        switch (video_mode)
        {
        case VM_LORES_MIXED:
            if (vcount >= 160)
                goto output_text;
            goto output_lores;

        case VM_HIRES_MIXED:
            if (vcount >= 160)
                goto output_text;
            goto output_hires;

        default:
        case VM_TEXT40:
        output_text:
            video_idx = (*cpu->rd->char_rom_data)[8 * video_byte + (vcount & 7)];

            // for inverse, xor the pixels with 0xFF to invert them.
            if ((video_byte & 0xC0) == 0) {  // inverse
                video_idx ^= 0xFF;
            } else if (((video_byte & 0xC0) == 0x40)) {  // flash
                video_idx ^= ds->flash_state ? 0xFF : 0;
            }

            video_bits = ds->text40_bits[video_idx];
            break;

        case VM_LORES:
        output_lores:
            ds->kill_color = false;
            video_idx = video_byte;
            video_idx = (video_idx >> (vcount & 4)) & 0x0F;  // hi or lo nibble
            video_idx = (video_idx << 1) | (hcount & 1); // even/odd columns
            video_bits = ds->lgr_bits[video_idx];
            break;

        case VM_HIRES:
        output_hires:
            ds->kill_color = false;
            video_idx = video_byte;
            video_bits = ds->hgr_bits[video_idx];
            break;
        }

        count = 14;
        rawbits = rawbits | video_bits; // carryover from HGR shifted bytes

        // initialize NTSC LUT index at the beginning of each scan line
        if (hcount == 0) {
            for (int i = NUM_TAPS; i; --i)
            {
                ntscidx = ntscidx >> 1;
                if (rawbits & 1)
                    ntscidx = ntscidx | (1 << ((NUM_TAPS*2)));
                rawbits = rawbits >> 1;
                --count;
            }
        }

        // this section produces the indices for the NTSC LUT, uses them
        // to grab RGB pixels, and stores the RGB pixels in the output texture
        for ( ; count; --count)
        {
            ntscidx = ntscidx >> 1;
            if (rawbits & 1)
                ntscidx = ntscidx | (1 << ((NUM_TAPS*2)));
            rawbits = rawbits >> 1;

            //  Use the phase and the bits as the index
            *outputImage++ = g_hgr_LUT[phase][ntscidx];
            phase = (phase+1) & 3;
        }

        if (++hcount == 40) {
            // output trailing pixels at the end of each line
            for (int count = NUM_TAPS; count; --count) {
                ntscidx = ntscidx >> 1;
                *outputImage++ = g_hgr_LUT[phase][ntscidx];
                phase = (phase+1) & 3;
            }

            // reset values for the beginning of the next line
            hcount = 0;
            rawbits = 0;
            ntscidx = 0;

            ++vcount;
        }
    }

    vs->end_video_cycle();
}

/**
 * Mono mode - just convert the bitstream to RGBA white (for now, I will grab the mono color later)
 */
void newProcessAppleIIFrame_Mono (
    cpu_state *cpu,             // access to cpu->vidbits
    RGBA_t* outputImage,          // Will be filled with 560x192 RGBA pixels
    RGBA_t color_value
)
{
    static RGBA_t p_black = { .a=0xFF, .b=0x00, .g=0x00, .r=0x00 };

    VideoScannerII *vs = cpu->get_video_scanner();
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    ds->kill_color = true;

    uint8_t  video_idx;
    uint16_t video_bits;
    uint16_t rawbits = 0;
    uint32_t vcount = 0;
    uint32_t hcount = 0;

    uint8_t * video_data = vs->get_video_data();
    int video_data_size = vs->get_video_data_size();
    int i = 0;

    while (i < video_data_size)
    {
        // This section builds a 14/15 bit wide video_bits for each byte of
        // video memory, based on the video_mode associated with each byte

        video_mode_t video_mode = (video_mode_t)(video_data[i++]);
        uint8_t video_byte = video_data[i++];

        switch (video_mode)
        {
        case VM_LORES_MIXED:
            if (vcount >= 160)
                goto output_text;
            goto output_lores;

        case VM_HIRES_MIXED:
            if (vcount >= 160)
                goto output_text;
            goto output_hires;

        default:
        case VM_TEXT40:
        output_text:
            video_idx = (*cpu->rd->char_rom_data)[8 * video_byte + (vcount & 7)];

            // for inverse, xor the pixels with 0xFF to invert them.
            if ((video_byte & 0xC0) == 0) {  // inverse
                video_idx ^= 0xFF;
            } else if (((video_byte & 0xC0) == 0x40)) {  // flash
                video_idx ^= ds->flash_state ? 0xFF : 0;
            }

            video_bits = ds->text40_bits[video_idx];
            break;

        case VM_LORES:
        output_lores:
            ds->kill_color = false;
            video_idx = video_byte;
            video_idx = (video_idx >> (vcount & 4)) & 0x0F;  // hi or lo nibble
            video_idx = (video_idx << 1) | (hcount & 1); // even/odd columns
            video_bits = ds->lgr_bits[video_idx];
            break;

        case VM_HIRES:
        output_hires:
            ds->kill_color = false;
            video_idx = video_byte;
            video_bits = ds->hgr_bits[video_idx];
            break;
        }

        rawbits = rawbits | video_bits; // carryover from HGR shifted bytes

        for (int i = 14; i; --i) {
            if (rawbits & 1)
                *outputImage++ = color_value;
            else
                *outputImage++ = p_black;
            rawbits >>= 1; 
        }

        if (++hcount == 40) {
            hcount = 0;
            rawbits = 0;
            ++vcount;
        }
    }

    vs->end_video_cycle();
}

void RGB_shift_color (uint16_t vbits, RGBA_t* outputImage)
{
    int col1 = 0;
    int col2 = 0;
    switch (vbits) {
        case 0:  col1 = col2 = 0;  break; /* black */
        case 1:  col1 = col2 = 0;  break; /* black */
        case 2:  col1 = col2 = 6;  break; /* blue */
        case 3:  col1 = 15; col2 = 0; break; /* white, black */
        case 4:  col1 = col2 = 9;  break; /* orange */
        case 5:  col1 = col2 = 9;  break; /* orange */
        case 6:  col1 = col2 = 15; break; /* white */
        case 7:  col1 = col2 = 15; break; /* white */
        case 8:  col1 = col2 = 0;  break; /* black */
        case 9:  col1 = col2 = 0;  break; /* black */
        case 10: col1 = col2 = 6;  break; /* blue */
        case 11: col1 = 15; col2 = 0;  break; /* white, black */
        case 12: col1 = 0;  col2 = 15; break; /* black, white */
        case 13: col1 = 0;  col2 = 15; break; /* black, white */
        case 14: col1 = col2 = 15; break; /* white */
        case 15: col1 = col2 = 15; break; /* white */
    }
    *outputImage++ = standard_iigs_color_table[col1];
    *outputImage++ = standard_iigs_color_table[col1];
    *outputImage++ = standard_iigs_color_table[col2];
    *outputImage++ = standard_iigs_color_table[col2];
}

void RGB_noshift_color (uint16_t vbits, RGBA_t* outputImage)
{
    int col1 = 0;
    int col2 = 0;
    switch (vbits) {
        case 0:  col1 = col2 = 0;  break; /* black */
        case 1:  col1 = col2 = 0;  break; /* black */
        case 2:  col1 = col2 = 3; break; /* purple */
        case 3:  col1 = 15; col2 = 0; break; /* white, black */
        case 4:  col1 = col2 = 12;  break; /* green */
        case 5:  col1 = col2 = 12;  break; /* green */
        case 6:  col1 = col2 = 15; break; /* white */
        case 7:  col1 = col2 = 15; break; /* white */
        case 8:  col1 = col2 = 0;  break; /* black */
        case 9:  col1 = col2 = 0;  break; /* black */
        case 10: col1 = col2 = 3; break; /* purple */
        case 11: col1 = 15; col2 = 0;  break; /* white, black */
        case 12: col1 = 0;  col2 = 15; break; /* black, white */
        case 13: col1 = 0;  col2 = 15; break; /* black, white */
        case 14: col1 = col2 = 15; break; /* white */
        case 15: col1 = col2 = 15; break; /* white */
    }
    *outputImage++ = standard_iigs_color_table[col1];
    *outputImage++ = standard_iigs_color_table[col1];
    *outputImage++ = standard_iigs_color_table[col2];
    *outputImage++ = standard_iigs_color_table[col2];
}

void newProcessAppleIIFrame_RGB (
    cpu_state *cpu,             // access to cpu->vidbits
    RGBA_t* outputImage         // Will be filled with 560x192 RGBA pixels
)
{
    VideoScannerII *vs = cpu->get_video_scanner();
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    ds->kill_color = true;

    uint8_t  video_idx;
    uint16_t video_bits;
    uint16_t rawbits = 0;
    uint32_t vcount = 0;
    uint32_t hcount = 0;
    uint8_t  last_byte = 0;
    uint8_t  last_shift = 0;

    uint8_t * video_data = vs->get_video_data();
    int video_data_size = vs->get_video_data_size();
    int i = 0;

    while (i < video_data_size)
    {
        // This section builds a 14/15 bit wide video_bits for each byte of
        // video memory, based on the video_mode associated with each byte

        video_mode_t video_mode = (video_mode_t)(video_data[i++]);
        uint8_t video_byte = video_data[i++];

        switch (video_mode)
        {
        case VM_LORES_MIXED:
            if (vcount >= 160)
                goto output_text;
            goto output_lores;

        case VM_HIRES_MIXED:
            if (vcount >= 160)
                goto output_text;
            goto output_hires;

        default:
        case VM_TEXT40:
        output_text:
            video_idx = (*cpu->rd->char_rom_data)[8 * video_byte + (vcount & 7)];

            // for inverse, xor the pixels with 0xFF to invert them.
            if ((video_byte & 0xC0) == 0) {  // inverse
                video_idx ^= 0xFF;
            } else if (((video_byte & 0xC0) == 0x40)) {  // flash
                video_idx ^= ds->flash_state ? 0xFF : 0;
            }

            video_bits = ds->text40_bits[video_idx];

            for (int i = 14; i; --i) {
                *outputImage++ = standard_iigs_color_table[(16 - (video_bits & 1)) & 15];
                video_bits >>= 1;
            }
            break;

        case VM_LORES:
        output_lores:
            ds->kill_color = false;
            video_idx = (video_byte >> (vcount & 4)) & 0x0F;  // hi or lo nibble
            for (int i = 14; i; --i)
                *outputImage++ = standard_iigs_color_table[video_idx];
            break;

        case VM_HIRES:
        output_hires:
            {
                ds->kill_color = false;

                uint8_t  shift = video_byte & 0x80;
                uint16_t vbits = video_byte;
                int col1  = 0;
                int col2  = 0;
                int count = 0;

                if (hcount & 1) {
                    vbits = (vbits << 2) | (last_byte >> 5);
                    count = 3;
                    if (hcount == 39)
                        count = 4;
                }
                else {
                    if (hcount) {
                        vbits = (vbits << 3) | (last_byte >> 4);
                        count = 4;
                    }
                    else {
                        vbits = (vbits << 1);
                        count = 3;
                    }
                }

                if (shift) {
                    for (int i = count; i; --i) {
                        RGB_shift_color(vbits & 15, outputImage);
                        vbits >>= 2;
                        outputImage += 4;
                    }
                }
                else {
                    if (last_shift) {
                        RGB_shift_color(vbits & 15, outputImage);
                        vbits >>= 2;
                        outputImage += 4;
                        --count;
                    }
                    for (int i = count; i; --i) {
                        RGB_noshift_color(vbits & 15, outputImage);
                        vbits >>= 2;
                        outputImage += 4;
                    }
                }

                last_byte = video_byte & 0x7F;
                last_shift = video_byte & 0x80;
                break;
            }

        }

        if (++hcount == 40) {
            hcount = 0;
            rawbits = 0;
            last_byte = 0;
            ++vcount;
        }
    }

    vs->end_video_cycle();
}

