
#include "display/DisplayRGB.hpp"

static int RGB_unshifted_color_1[16] = {
    0,0,3,15,12,12,15,15,
    0,0,3,15,0,12,15,15
};

static int RGB_unshifted_color_2[16] = {
    0,0,3,0,12,12,15,15,
    0,0,3,3,15,15,15,15
};

static int RGB_shifted_color_1[16] = {
    0,0,6,15,9,9,15,15,
    0,0,6,15,0,9,15,15
};

static int RGB_shifted_color_2[16] = {
    0,0,6,0,9,9,15,15,
    0,0,6,6,15,15,15,15
};

bool DisplayRGB::update_display(cpu_state *cpu)
{
    VideoScannerII *vs = cpu->get_video_scanner();

    uint8_t  video_rom_data;
    uint16_t video_bits;
    uint16_t rawbits = 0;
    uint32_t vcount = 0;
    uint32_t hcount = 0;
    uint8_t  last_byte = 0;
    output = buffer;

    uint8_t * video_data = vs->get_video_data();
    int video_data_size = vs->get_video_data_size();

    int i = 0;
    while (i < video_data_size)
    {
        if (hcount == 0) {
            output += 7;
        }

        // This section builds a 14/15 bit wide video_bits for each byte of
        // video memory, based on the video_mode associated with each byte

        video_mode_t video_mode = (video_mode_t)(video_data[i++]);
        uint8_t video_byte = video_data[i++];

        if (video_mode == VM_LAST_HBL) {
            // ignore this
            video_mode = (video_mode_t)(video_data[i++]);
            video_byte = video_data[i++];
        }

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
        output_text40:
            video_rom_data = (*cpu->rd->char_rom_data)[8 * video_byte + (vcount & 7)];

            if (computer->platform->id < PLATFORM_APPLE_IIE) {
                // for inverse, xor the pixels with 0xFF to invert them.
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
            for (int i = 14; i; --i) {
                *output++ = iigs_color_table[(16 - (video_bits & 1)) & 15];
                video_bits >>= 1;
            }
            break;

        case VM_ALT_TEXT40:
        output_alt_text40:
            video_rom_data = 0xFF ^ (*cpu->rd->char_rom_data)[8 * video_byte + (vcount & 7)];
            video_bits = 0;
            for (int i = 0; i < 7; ++i) {
                video_bits = (video_bits >> 1) | ((video_rom_data & 1) << 13);
                video_bits = (video_bits >> 1) | ((video_rom_data & 1) << 13);
                video_rom_data >>= 1;
            }
            for (int i = 14; i; --i) {
                *output++ = iigs_color_table[(16 - (video_bits & 1)) & 15];
                video_bits >>= 1;
            }
            break;

        case VM_TEXT80:
        output_text80:
            video_rom_data = 0xFF ^ (*cpu->rd->char_rom_data)[8 * video_byte + (vcount & 7)];
            if ((video_byte & 0xC0) == 0x40)
                video_rom_data ^= flash_mask();
            for (int i = 7; i; --i) {
                *output++ = iigs_color_table[(16 - (video_rom_data & 1)) & 15];
                video_rom_data >>= 1;
            }
            video_byte = video_data[i++];
            video_rom_data = 0xFF ^ (*cpu->rd->char_rom_data)[8 * video_byte + (vcount & 7)];
            if ((video_byte & 0xC0) == 0x40)
                video_rom_data ^= flash_mask();
            for (int i = 7; i; --i) {
                *output++ = iigs_color_table[(16 - (video_rom_data & 1)) & 15];
                video_rom_data >>= 1;
            }
            break;

        case VM_ALT_TEXT80:
        output_alt_text80:
            video_rom_data = 0xFF ^ (*cpu->rd->char_rom_data)[8 * video_byte + (vcount & 7)];
            for (int i = 7; i; --i) {
                *output++ = iigs_color_table[(16 - (video_rom_data & 1)) & 15];
                video_rom_data >>= 1;
            }
            video_byte = video_data[i++];
            video_rom_data = 0xFF ^ (*cpu->rd->char_rom_data)[8 * video_byte + (vcount & 7)];
            for (int i = 7; i; --i) {
                *output++ = iigs_color_table[(16 - (video_rom_data & 1)) & 15];
                video_rom_data >>= 1;
            }
            break;

        case VM_LORES:
        output_lores:
            video_byte = (video_byte >> (vcount & 4)) & 0x0F;  // hi or lo nibble
            for (int i = 14; i; --i)
                *output++ = iigs_color_table[video_byte];
            break;

        case VM_HIRES:
        output_hires:
            {
                int col1, col2;

                uint16_t video_bits = 0;
                uint8_t shift = video_byte & 0x80;

                video_mode_t next_mode = (video_mode_t)(video_data[i]);
                uint16_t next_byte = video_data[i+1];

                bool next_is_hgr = false;
                if (next_mode == VM_HIRES) next_is_hgr = true;
                else if (next_mode == VM_HIRES_MIXED) next_is_hgr = true;
                else if (next_mode == VM_HIRES_MIXED80) next_is_hgr = true;
                else if (next_mode == VM_HIRES_ALT_MIXED) next_is_hgr = true;
                else if (next_mode == VM_HIRES_ALT_MIXED80) next_is_hgr = true;

                video_byte = video_byte & 0x7F;

                if (hcount == 0) {
                    video_bits = video_byte;
                    video_bits = video_bits << 1;
                    if (next_is_hgr)
                        video_bits = video_bits | (next_byte << 8);
                }
                else if (hcount & 1) {
                    video_bits = video_byte;
                    video_bits = video_bits << 2;
                    video_bits = video_bits | (last_byte >> 5);
                    if (next_is_hgr)
                        video_bits = video_bits | (next_byte << 9);
                }
                else {
                    video_bits = video_byte;
                    video_bits = video_bits << 1;
                    video_bits = video_bits | (last_byte >> 6);
                    if (next_is_hgr)
                        video_bits = video_bits | (next_byte << 8);
                }

                if (hcount & 1) {
                    output -= 2;
                    for (int count = 4; count; --count) {
                        if (shift) {
                            col1 = RGB_shifted_color_1[video_bits & 15];
                            col2 = RGB_shifted_color_2[video_bits & 15];
                        }
                        else {
                            col1 = RGB_unshifted_color_1[video_bits & 15];
                            col2 = RGB_unshifted_color_2[video_bits & 15];
                        }
                        *output++ = iigs_color_table[col1];
                        *output++ = iigs_color_table[col1];
                        *output++ = iigs_color_table[col2];
                        *output++ = iigs_color_table[col2];
                        video_bits = video_bits >> 2;
                    }
                }
                else {
                    for (int count = 4; count; --count) {
                        if (shift) {
                            col1 = RGB_shifted_color_1[video_bits & 15];
                            col2 = RGB_shifted_color_2[video_bits & 15];
                        }
                        else {
                            col1 = RGB_unshifted_color_1[video_bits & 15];
                            col2 = RGB_unshifted_color_2[video_bits & 15];
                        }
                        *output++ = iigs_color_table[col1];
                        *output++ = iigs_color_table[col1];
                        *output++ = iigs_color_table[col2];
                        *output++ = iigs_color_table[col2];
                        video_bits = video_bits >> 2;
                    }
                    output -= 2;
                }

                last_byte = video_byte & 0x7F;
            }
            break;

        case VM_DLORES:
        output_dlores:
            video_byte = video_data[i++]; // skip aux byte
            goto output_lores;
            break;

        case VM_DHIRES:
        output_dhires:
            video_byte = video_data[i++]; // skip aux byte
            goto output_hires;
            break;
        }

        if (++hcount == 40) {
            hcount = 0;
            rawbits = 0;
            last_byte = 0;
            ++vcount;
        }
    }

    return Display::update_display(cpu);
}

DisplayRGB::DisplayRGB(computer_t *computer) : Display(computer)
{
}

void init_mb_display_rgb(computer_t *computer, SlotType_t slot) {
    // alloc and init Display
    DisplayRGB *ds = new DisplayRGB(computer);
    printf("rgb disp:%p\n", ds); fflush(stdout);
    ds->register_display_device(computer, DEVICE_ID_DISPLAY_RGB);
}
