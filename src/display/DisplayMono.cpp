
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

#if 0
bool DisplayMono::update_display(cpu_state *cpu)
{
    uint16_t rawbits = 0;
    output = buffer;

    hcount = 0;
    vcount = 0;

    begin_video_bits(cpu);
    while (more_video_bits())
    {   
        uint16_t video_bits = next_video_bits(cpu);

        // carryover from HGR shifted bytes
        rawbits = rawbits | video_bits;

        for (int i = 14; i; --i) {
            if (rawbits & 1)
                *output++ = phosphor_color;
            else
                *output++ = p_black;
            rawbits >>= 1; 
        }
        
        if (++hcount == 40) {
            hcount = 0;
            rawbits = 0;
            ++vcount;
        }
    }
    return Display::update_display(cpu);
}
#endif

void init_mb_display_mono(computer_t *computer, SlotType_t slot) {
    // alloc and init Display
    DisplayMono *ds = new DisplayMono(computer);
    printf("mono disp:%p\n", ds); fflush(stdout);
    ds->register_display_device(computer, DEVICE_ID_DISPLAY_MONO);
}
