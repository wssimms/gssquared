#pragma once

#include <cstdint>

//typedef int int;
typedef uint8_t Buffer[];

/**
 * We have three different data types for image data. Maybe four.
 * 1. RGBA - final output for emulator and input into PPM writer. four unsigned 8-bit elements.
 * 2. YIQ - intermedia form - three float elements.
 * 3. Mono - single unsigned 8-bit element. Just a luminance value, integer, 0-255.
 * 4. 
 *  */ 

struct RGBA {
    //uint8_t r, g, b, a;
    uint8_t a, b, g, r; // has to be in this order to match the SDL3 pixel format.
};
