#pragma once

#include "display/DisplayComposite.hpp"

class DisplayMono : public DisplayComposite
{
public:
    DisplayMono(computer_t * computer);

    bool update_display(cpu_state *cpu) override;
};

void init_mb_display_mono(computer_t *computer, SlotType_t slot);
