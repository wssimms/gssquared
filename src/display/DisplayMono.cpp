
#include "display/DisplayMono.hpp"
#include "Device_ID.hpp"

DisplayMono::DisplayMono(computer_t * computer) : DisplayComposite(computer)
{
    phosphor_color = p_white;
}

void DisplayMono::set_color_white()
{
    phosphor_color = p_white;
}

void DisplayMono::set_color_amber()
{
    phosphor_color = p_amber;
}

void DisplayMono::set_color_green()
{
    phosphor_color = p_green;
}


bool DisplayMono::update_display(cpu_state *cpu)
{
    output = buffer;

    begin_video_bits(cpu);
    for (vcount = 0; vcount < 192; ++vcount)
    {
        build_scanline(cpu, vcount);

        for (int n = 0; n < 81; ++n) {
            uint8_t video_bits = scanline[n];
            for (int i = 7; i; --i) {
                if (video_bits & 1)
                    *output++ = phosphor_color;
                else
                    *output++ = p_black;
                video_bits >>= 1; 
            }
        }
    }

    return Display::update_display(cpu);
}

void init_mb_display_mono(computer_t *computer, SlotType_t slot) {
    // alloc and init Display
    DisplayMono *ds = new DisplayMono(computer);

    computer->register_shutdown_handler([ds]() {
        delete ds;
        return true;
    });

    ds->register_display_device(computer, DEVICE_ID_DISPLAY_MONO);
}
