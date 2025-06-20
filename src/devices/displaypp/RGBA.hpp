#pragma once

#include <cstdint>

#pragma pack(push, 1)

#ifdef __LITTLE_ENDIAN__
struct RGBA_t {
    union {
        struct {
//            uint8_t r, g, b, a;
            uint8_t a, b, g, r; // to match current inefficient pixel format.

        };
        uint32_t rgba;
    };
};
#else
struct RGBA_t {
    union {
        struct {
            uint8_t a, b, g, r;
        }
        uint32_t rgba;
    };
};
#endif

#pragma pack(pop)
