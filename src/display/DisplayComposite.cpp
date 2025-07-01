
#include "display/DisplayComposite.hpp"
#include "display/display.hpp"
#include "platforms.hpp"

void DisplayComposite::build_scanline(cpu_state *cpu, unsigned vcount)
{
    uint8_t video_byte;
    uint8_t video_rom_data;
    uint16_t video_bits;
    uint16_t last_hgr_bit;

    video_mode_t video_mode = (video_mode_t)(video_data[idx++]);

    if (video_mode != VM_LAST_HBL) {
        printf("OH NO! There isn't a VM_LAST_HBL byte before this scanline.\n");
    }
    else {
        video_byte = video_data[idx++];
        last_hgr_bit = (video_byte >> 6) & 1;
    }

    int nbytes = 0;
    while (nbytes < 80) {
        video_mode = (video_mode_t)(video_data[idx++]);

        switch (video_mode)
        {
        case VM_LORES_MIXED:
            if (vcount >= 160)
                goto output_text40;
            goto output_lores;

        case VM_LORES_ALT_MIXED:
            if (vcount >= 160)
                goto output_alt_text40;
            goto output_lores;

        case VM_HIRES_MIXED:
            if (vcount >= 160)
                goto output_text40;
            goto output_hires;

        case VM_HIRES_ALT_MIXED:
            if (vcount >= 160)
                goto output_alt_text40;
            goto output_hires;

        case VM_HIRES_NOSHIFT_MIXED:
            if (vcount >= 160)
                goto output_text40;
            goto output_hires_noshift;

        case VM_HIRES_NOSHIFT_ALT_MIXED:
            if (vcount >= 160)
                goto output_alt_text40;
            goto output_hires_noshift;

        case VM_LORES_MIXED80:
            if (vcount >= 160)
                goto output_text80;
            goto output_lores;

        case VM_LORES_ALT_MIXED80:
            if (vcount >= 160)
                goto output_alt_text80;
            goto output_lores;

        case VM_HIRES_MIXED80:
            if (vcount >= 160)
                goto output_text80;
            goto output_hires;

        case VM_HIRES_ALT_MIXED80:
            if (vcount >= 160)
                goto output_alt_text80;
            goto output_hires;

        case VM_DLORES_MIXED80:
            if (vcount >= 160)
                goto output_text80;
            goto output_dlores;

        case VM_DLORES_ALT_MIXED80:
            if (vcount >= 160)
                goto output_alt_text80;
            goto output_dlores;

        case VM_DHIRES_MIXED80:
            if (vcount >= 160)
                goto output_text80;
            goto output_dhires;

        case VM_DHIRES_ALT_MIXED80:
            if (vcount >= 160)
                goto output_alt_text80;
            goto output_dhires;

        case VM_TEXT40:
        output_text40:
        default:
            if ((nbytes & 1) == 0)
                scanline[nbytes++] = 0;

            video_byte = video_data[idx++];
            video_rom_data = (*cpu->rd->char_rom_data)[8 * video_byte + (vcount & 7)];
            if (computer->platform->id < PLATFORM_APPLE_IIE) {
                if ((video_byte & 0xC0) == 0) {  // inverse
                    video_rom_data ^= 0xFF;
                } else if (((video_byte & 0xC0) == 0x40)) {  // flash
                    video_rom_data ^= flash_mask();
                }

                video_bits = text40_bits[video_rom_data];
            }
            else {
                video_rom_data ^= 0xFF;
                if ((video_byte & 0xC0) == 0x40) {  // flash
                    video_rom_data ^= flash_mask();
                }

                video_bits = 0;
                for (int i = 0; i < 7; ++i) {
                    video_bits = (video_bits >> 1) | ((video_rom_data & 1) << 13);
                    video_bits = (video_bits >> 1) | ((video_rom_data & 1) << 13);
                    video_rom_data >>= 1;
                }
            }
            scanline[nbytes++] = video_bits & 0x7F;
            scanline[nbytes++] = (video_bits >> 7) & 0x7F;
            break;

        case VM_ALT_TEXT40:
        output_alt_text40:
            if ((nbytes & 1) == 0)
                scanline[nbytes++] = 0;
                
            video_byte = video_data[idx++];
            video_rom_data = 0xFF ^ (*cpu->rd->char_rom_data)[8 * video_byte + (vcount & 7)];
            video_bits = 0;
            for (int i = 0; i < 7; ++i) {
                video_bits = (video_bits >> 1) | ((video_rom_data & 1) << 13);
                video_bits = (video_bits >> 1) | ((video_rom_data & 1) << 13);
                video_rom_data >>= 1;
            }

            scanline[nbytes++] = video_bits & 0x7F;
            scanline[nbytes++] = (video_bits >> 7) & 0x7F;
            break;

        case VM_TEXT80:
        output_text80:
            video_byte = video_data[idx++];
            video_rom_data = 0xFF ^ (*cpu->rd->char_rom_data)[8 * video_byte + (vcount & 7)];
            if ((video_byte & 0xC0) == 0x40)
                video_rom_data ^= flash_mask();
            scanline[nbytes++] = video_rom_data & 0x7F;
            video_byte = video_data[idx++];
            video_rom_data = 0xFF ^ (*cpu->rd->char_rom_data)[8 * video_byte + (vcount & 7)];
            if ((video_byte & 0xC0) == 0x40)
                video_rom_data ^= flash_mask();
            scanline[nbytes++] = video_rom_data & 0x7F;
            break;

        case VM_ALT_TEXT80:
        output_alt_text80:
            video_byte = video_data[idx++];
            video_rom_data = 0xFF ^ (*cpu->rd->char_rom_data)[8 * video_byte + (vcount & 7)];
            scanline[nbytes++] = video_rom_data & 0x7F;
            video_byte = video_data[idx++];
            video_rom_data = 0xFF ^ (*cpu->rd->char_rom_data)[8 * video_byte + (vcount & 7)];
            scanline[nbytes++] = video_rom_data & 0x7F;
            break;

        case VM_LORES:
        output_lores:
            text_count = 0;
            if ((nbytes & 1) == 0)
                scanline[nbytes++] = 0;

            video_byte = video_data[idx++];
            video_byte = (video_byte >> (vcount & 4)) & 0x0F;  // hi or lo nibble
            video_byte = (video_byte << 1) | ((nbytes >> 1) & 1); // even/odd columns
            video_bits = lgr_bits[video_byte];
            scanline[nbytes++] = video_bits & 0x7F;
            scanline[nbytes++] = (video_bits >> 7) & 0x7F;
            break;

        case VM_HIRES:
        output_hires:
            text_count = 0;
            if ((nbytes & 1) == 0)
                scanline[nbytes++] = 0;
            
            video_byte = video_data[idx++];
            video_bits = hgr_bits[video_byte];
            if (video_byte & 0x80)
                video_bits = video_bits | last_hgr_bit;
            last_hgr_bit = (video_bits >> 13) & 1;
            scanline[nbytes++] = video_bits & 0x7F;
            scanline[nbytes++] = (video_bits >> 7) & 0x7F;
            break;

        case VM_HIRES_NOSHIFT:
        output_hires_noshift:
            text_count = 0;
            if ((nbytes & 1) == 0)
                scanline[nbytes++] = 0;
            
            video_byte = video_data[idx++];
            video_bits = hgr_bits[video_byte & 0x7F];
            scanline[nbytes++] = video_bits & 0x7F;
            scanline[nbytes++] = (video_bits >> 7) & 0x7F;
            break;

        case VM_DLORES:
        output_dlores:
            text_count = 0;
            // aux byte
            video_byte = video_data[idx++];
            video_byte = (video_byte >> (vcount & 4)) & 0x0F;  // hi or lo nibble
            video_byte = (video_byte << 1) | ((nbytes >> 1) & 1); // even/odd columns
            video_bits = lgr_bits[video_byte];
            scanline[nbytes++] = video_bits & 0x7F;
            //scanline[nbytes++] = (video_bits >> 7) & 0x7F;
            // main byte
            video_byte = video_data[idx++];
            video_byte = (video_byte >> (vcount & 4)) & 0x0F;  // hi or lo nibble
            video_byte = (video_byte << 1) | ((nbytes >> 1) & 1); // even/odd columns
            video_bits = lgr_bits[video_byte];
            scanline[nbytes++] = video_bits & 0x7F;
            break;

        case VM_DHIRES:
        output_dhires:
            text_count = 0;
            scanline[nbytes++] = video_data[idx++] & 0x7F;
            scanline[nbytes++] = video_data[idx++] & 0x7F;
            break;
        }
    }

    if (nbytes == 80)
        scanline[nbytes] = 0;
}

void DisplayComposite::begin_video_bits(cpu_state *cpu)
{
    idx = 0;
    video_data = cpu->get_video_scanner()->get_video_data();
    video_data_size = cpu->get_video_scanner()->get_video_data_size();
    if (text_count < 4)
        text_count += 1;
}

DisplayComposite::DisplayComposite(computer_t *computer) : Display(computer)
{
    text_count = 0;
}
