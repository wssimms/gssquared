#pragma once

#include "display/DisplayComposite.hpp"
#include "display/filters.hpp"

class DisplayTV : public DisplayComposite
{
private:
    RGBA_t ntsc_lut[4][(1 << ((NUM_TAPS * 2) + 1))];

    RGBA_t ntsc_lut_color(uint32_t inputBits, int pixelposition);
    void   init_ntsc_lut();

public:
    DisplayTV(computer_t * computer);

    bool update_display(cpu_state *cpu) override;
};

void init_mb_display_tv(computer_t *computer, SlotType_t slot);
