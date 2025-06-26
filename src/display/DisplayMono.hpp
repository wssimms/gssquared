#pragma once

#include "display/DisplayComposite.hpp"

class DisplayMono : public DisplayComposite
{
private:
    RGBA_t phosphor_color;
    
public:
    DisplayMono(computer_t * computer);
    bool update_display(cpu_state *cpu) override;
    void set_color_white();
    void set_color_amber();
    void set_color_green();
};

void init_mb_display_mono(computer_t *computer, SlotType_t slot);
