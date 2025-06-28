
#include "display/DisplayRGB.hpp"

void RGB_shift_color (uint16_t vbits, RGBA_t* outputImage)
{
    int col1 = 0;
    int col2 = 0;
    switch (vbits) {
        case 0:  col1 = col2 = 0;  break; /* black */
        case 1:  col1 = col2 = 0;  break; /* black */
        case 2:  col1 = col2 = 6;  break; /* blue */
        case 3:  col1 = 15; col2 = 0; break; /* white, black */
        case 4:  col1 = col2 = 9;  break; /* orange */
        case 5:  col1 = col2 = 9;  break; /* orange */
        case 6:  col1 = col2 = 15; break; /* white */
        case 7:  col1 = col2 = 15; break; /* white */
        case 8:  col1 = col2 = 0;  break; /* black */
        case 9:  col1 = col2 = 0;  break; /* black */
        case 10: col1 = col2 = 6;  break; /* blue */
        case 11: col1 = 15; col2 = 0;  break; /* white, black */
        case 12: col1 = 0;  col2 = 15; break; /* black, white */
        case 13: col1 = 0;  col2 = 15; break; /* black, white */
        case 14: col1 = col2 = 15; break; /* white */
        case 15: col1 = col2 = 15; break; /* white */
    }
    *outputImage++ = iigs_color_table[col1];
    *outputImage++ = iigs_color_table[col1];
    *outputImage++ = iigs_color_table[col2];
    *outputImage++ = iigs_color_table[col2];
}

void RGB_noshift_color (uint16_t vbits, RGBA_t* outputImage)
{
    int col1 = 0;
    int col2 = 0;
    switch (vbits) {
        case 0:  col1 = col2 = 0;  break; /* black */
        case 1:  col1 = col2 = 0;  break; /* black */
        case 2:  col1 = col2 = 3; break; /* purple */
        case 3:  col1 = 15; col2 = 0; break; /* white, black */
        case 4:  col1 = col2 = 12;  break; /* green */
        case 5:  col1 = col2 = 12;  break; /* green */
        case 6:  col1 = col2 = 15; break; /* white */
        case 7:  col1 = col2 = 15; break; /* white */
        case 8:  col1 = col2 = 0;  break; /* black */
        case 9:  col1 = col2 = 0;  break; /* black */
        case 10: col1 = col2 = 3; break; /* purple */
        case 11: col1 = 15; col2 = 0;  break; /* white, black */
        case 12: col1 = 0;  col2 = 15; break; /* black, white */
        case 13: col1 = 0;  col2 = 15; break; /* black, white */
        case 14: col1 = col2 = 15; break; /* white */
        case 15: col1 = col2 = 15; break; /* white */
    }
    *outputImage++ = iigs_color_table[col1];
    *outputImage++ = iigs_color_table[col1];
    *outputImage++ = iigs_color_table[col2];
    *outputImage++ = iigs_color_table[col2];
}

bool DisplayRGB::update_display(cpu_state *cpu)
{
    VideoScannerII *vs = cpu->get_video_scanner();

    uint8_t  video_idx;
    uint16_t video_bits;
    uint16_t rawbits = 0;
    uint32_t vcount = 0;
    uint32_t hcount = 0;
    uint8_t  last_byte = 0;
    uint8_t  last_shift = 0;
    output = buffer;

    uint8_t * video_data = vs->get_video_data();
    int video_data_size = vs->get_video_data_size();

    int i = 0;
    while (i < video_data_size)
    {
        // This section builds a 14/15 bit wide video_bits for each byte of
        // video memory, based on the video_mode associated with each byte

        video_mode_t video_mode = (video_mode_t)(video_data[i++]);
        uint8_t video_byte = video_data[i++];

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

            for (int i = 14; i; --i) {
                *output++ = iigs_color_table[(16 - (video_bits & 1)) & 15];
                video_bits >>= 1;
            }
            break;

        case VM_LORES:
        output_lores:
            video_idx = (video_byte >> (vcount & 4)) & 0x0F;  // hi or lo nibble
            for (int i = 14; i; --i)
                *output++ = iigs_color_table[video_idx];
            break;

        case VM_HIRES:
        output_hires:
            {
                uint8_t  shift = video_byte & 0x80;
                uint16_t vbits = video_byte;
                int col1  = 0;
                int col2  = 0;
                int count = 0;

                if (hcount & 1) {
                    vbits = (vbits << 2) | (last_byte >> 5);
                    count = 3;
                    if (hcount == 39)
                        count = 4;
                }
                else {
                    if (hcount) {
                        vbits = (vbits << 3) | (last_byte >> 4);
                        count = 4;
                    }
                    else {
                        vbits = (vbits << 1);
                        count = 3;
                    }
                }

                if (shift) {
                    for (int i = count; i; --i) {
                        RGB_shift_color(vbits & 15, output);
                        vbits >>= 2;
                        output += 4;
                    }
                }
                else {
                    if (last_shift) {
                        RGB_shift_color(vbits & 15, output);
                        vbits >>= 2;
                        output += 4;
                        --count;
                    }
                    for (int i = count; i; --i) {
                        RGB_noshift_color(vbits & 15, output);
                        vbits >>= 2;
                        output += 4;
                    }
                }

                last_byte = video_byte & 0x7F;
                last_shift = video_byte & 0x80;
                break;
            }

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
