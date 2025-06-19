#pragma once

#include <cstdint>

#pragma pack(push, 1)
struct RGBA_t {
    union {
        struct {
            uint8_t r, g, b, a;
        };
        uint32_t rgba;
    };
};
#pragma pack(pop)
