#pragma once

#include "display/display.hpp"

class DisplayComposite : public Display
{
protected:
    constexpr static RGBA_t p_white = {.a=0xFF, .b=0xFF, .g=0xFF, .r=0xFF};
    constexpr static RGBA_t p_black = {.a=0xFF, .b=0x00, .g=0x00, .r=0x00};

    unsigned text_count;
    unsigned idx;
    unsigned hcount;
    unsigned vcount;
    unsigned video_data_size;
    uint8_t *video_data;

    inline bool kill_color() { return text_count >= 4; }

    uint16_t first_video_bits(cpu_state *cpu);
    uint16_t next_video_bits(cpu_state *cpu);
    bool     more_video_bits();

public:
    DisplayComposite(computer_t * computer);
    virtual bool update_display(cpu_state *cpu) = 0;
};
