
#include "display/DisplayComposite.hpp"
#include "display/display.hpp"
#include "platforms.hpp"

uint16_t DisplayComposite::next_video_bits(cpu_state *cpu)
{
    // This builds a 14/15 bit wide video_bits for each byte of
    // video memory, based on the video_mode associated with each byte

    video_mode_t video_mode = (video_mode_t)(video_data[idx++]);
    uint8_t video_byte = video_data[idx++];
    uint16_t video_bits = 0;
    uint16_t video_idx = 0;

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

    case VM_LORES_MIXED80:
        if (vcount >= 160)
            goto output_text80;
        goto output_lores;

    case VM_HIRES_MIXED80:
        if (vcount >= 160)
            goto output_text80;
        goto output_hires;

    case VM_DLORES_MIXED:
        if (vcount >= 160)
            goto output_text80;
        goto output_dlores;

    case VM_DHIRES_MIXED:
        if (vcount >= 160)
            goto output_text80;
        goto output_dhires;

    default:
    case VM_TEXT40:
    output_text:
        video_idx = (*cpu->rd->char_rom_data)[8 * video_byte + (vcount & 7)];

        if (computer->platform->id < PLATFORM_APPLE_IIE) {
            if ((video_byte & 0xC0) == 0) {  // inverse
                video_idx ^= 0xFF;
            } else if (((video_byte & 0xC0) == 0x40)) {  // flash
                video_idx ^= flash_mask();
            }

            video_bits = text40_bits[video_idx];
        }
        else {
            video_idx ^= 0xFF;

            if ((video_byte & 0xC0) == 0x40) {  // flash
                video_idx ^= flash_mask();
            }

            for (int i = 0; i < 7; ++i) {
                video_bits = (video_bits >> 1) | ((video_idx & 1) << 13);
                video_bits = (video_bits >> 1) | ((video_idx & 1) << 13);
                video_idx >>= 1;
            }
        }

        break;

    case VM_TEXT80:
    output_text80:
        // this is the byte from auxiliary memory
        video_idx = (*cpu->rd->char_rom_data)[8 * video_byte + (vcount & 7)];

        video_idx ^= 0xFF;
        if ((video_byte & 0xC0) == 0x40) {  // flash
            video_idx ^= flash_mask();
        }

        video_bits = video_idx;

        // now the byte from main memory
        video_byte = video_data[idx++];
        video_idx = (*cpu->rd->char_rom_data)[8 * video_byte + (vcount & 7)];

        video_idx ^= 0xFF;
        if ((video_byte & 0xC0) == 0x40) {  // flash
            video_idx ^= flash_mask();
        }

        video_bits = video_bits | (video_idx << 7);
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

    case VM_DLORES:
    output_dlores:
        video_byte = video_data[idx++]; // consume the auxmem byte
        goto output_lores;

    case VM_DHIRES:
    output_dhires:
        video_byte = video_data[idx++]; // consume the auxmem byte
        goto output_hires;
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
