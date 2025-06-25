
#include "display/DisplayMono.hpp"
#include "Device_ID.hpp"

DisplayMono::DisplayMono(computer_t * computer) : DisplayComposite(computer)
{
}

bool DisplayMono::update_display(cpu_state *cpu)
{
    uint16_t rawbits = 0;
    RGBA_t * output = buffer;

    hcount = 0;
    vcount = 0;

    uint16_t video_bits = first_video_bits(cpu);

    while (more_video_bits())
    {
        // carryover from HGR shifted bytes
        rawbits = rawbits | video_bits;

        for (int i = 14; i; --i) {
            if (rawbits & 1)
                *output++ = p_white;
            else
                *output++ = p_black;
            rawbits >>= 1; 
        }
        
        if (++hcount == 40) {
            hcount = 0;
            rawbits = 0;
            ++vcount;
        }

        video_bits = next_video_bits(cpu);
    }

    return Display::update_display(cpu);
}

void init_mb_display_mono(computer_t *computer, SlotType_t slot) {
    // alloc and init Display
    DisplayMono *ds = new DisplayMono(computer);
    printf("mono disp:%p\n", ds); fflush(stdout);
    ds->register_display_device(computer, DEVICE_ID_DISPLAY_MONO);
}
