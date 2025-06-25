
#include "display/DisplayComposite.hpp"
#include "display/display.hpp"

uint16_t DisplayComposite::next_video_bits(cpu_state *cpu)
{
    // This builds a 14/15 bit wide video_bits for each byte of
    // video memory, based on the video_mode associated with each byte

    video_mode_t video_mode = (video_mode_t)(video_data[idx++]);
    uint8_t video_byte = video_data[idx++];
    uint16_t video_bits = 0;
    uint8_t video_idx = 0;

    switch (video_mode)
    {
    case VM_LORES_MIXED:
        if (vcount >= 160)
            goto output_text;
        goto output_lores;

    case VM_HIRES_MIXED:
        if (vcount >= 160)
            goto output_text;
        goto output_hires;

    default:
    case VM_TEXT40:
    output_text:
        video_idx = (*cpu->rd->char_rom_data)[8 * video_byte + (vcount & 7)];

        // for inverse, xor the pixels with 0xFF to invert them.
        if ((video_byte & 0xC0) == 0) {  // inverse
            video_idx ^= 0xFF;
        } else if (((video_byte & 0xC0) == 0x40)) {  // flash
            video_idx ^= flash_mask();
        }

        video_bits = text40_bits[video_idx];
        break;

    case VM_LORES:
    output_lores:
        text_count = 0;
        video_idx = video_byte;
        video_idx = (video_idx >> (vcount & 4)) & 0x0F;  // hi or lo nibble
        video_idx = (video_idx << 1) | (hcount & 1); // even/odd columns
        video_bits = lgr_bits[video_idx];
        break;

    case VM_HIRES:
    output_hires:
        text_count = 0;
        video_idx = video_byte;
        video_bits = hgr_bits[video_idx];
        break;
    }

    return video_bits;
}

uint16_t DisplayComposite::first_video_bits(cpu_state *cpu)
{
    idx = 0;
    video_data = cpu->get_video_scanner()->get_video_data();
    video_data_size = cpu->get_video_scanner()->get_video_data_size();
    if (text_count < 4)
        text_count += 1;

    return next_video_bits(cpu);
}

bool DisplayComposite::more_video_bits()
{
    return idx < video_data_size;
}

DisplayComposite::DisplayComposite(computer_t *computer) : Display(computer)
{
    text_count = 0;
}
