#pragma once

#include <cstdint>

#pragma pack(push, 1)

/**
 * different platforms have different 'preferred' pixel formats.
 * we will select at compile time the best data structure.
 * This likely needs to be more like 'Mac' etc.
 */

#ifdef __APPLE__
// Apple - ABGR 
struct RGBA_t {
    union {
        struct {
            uint8_t r, g, b, a;
        };
        uint32_t rgba;
    };
};
#define PIXEL_FORMAT SDL_PIXELFORMAT_ABGR8888
#elif defined(__LINUX__)
// Linux - RGBA
struct RGBA_t {
    union {
        struct {
            uint8_t a, b, g, r;
        }
        uint32_t rgba;
    };
};
#define PIXEL_FORMAT SDL_PIXELFORMAT_RGBA8888
#elif defined(__WINDOWS__)
// Windows - BGRA
struct RGBA_t {
    union {
        struct {
            uint8_t a, r, g, b;
        }   
        uint32_t rgba;
    };
};
#define PIXEL_FORMAT SDL_PIXELFORMAT_BGRA8888
#else
#error "Unsupported platform"
#endif

#pragma pack(pop)
