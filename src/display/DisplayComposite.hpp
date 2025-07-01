#pragma once

#include "display/DisplayBase.hpp"

class DisplayComposite : public Display
{
protected:
    constexpr static RGBA_t p_white = {.a=0xFF, .b=0xFF, .g=0xFF, .r=0xFF};
    constexpr static RGBA_t p_green = {.a=0xFF, .b=0x4A, .g=0xFF, .r=0x00};
    constexpr static RGBA_t p_amber = {.a=0xFF, .b=0x00, .g=0xBF, .r=0xFF};
    constexpr static RGBA_t p_black = {.a=0xFF, .b=0x00, .g=0x00, .r=0x00};

    unsigned text_count;
    unsigned idx;
    unsigned hcount;
    unsigned vcount;
    unsigned video_data_size;
    uint8_t *video_data;

    inline bool kill_color() { return text_count >= 4; }

    uint8_t scanline[81]; // 81*7 == 567 pixels across
    void build_scanline(cpu_state *cpu, unsigned vcount);
    void begin_video_bits(cpu_state *cpu);

public:
    DisplayComposite(computer_t * computer);
    virtual bool update_display(cpu_state *cpu) = 0;
};
