
#include "display/DisplayComposite.hpp"
#include "util/EventDispatcher.hpp"
#include "videosystem.hpp"

ntsc_config config;

// Function to generate phase information for a scanline and stuff in config.
void generatePhaseInfo(int scanlineY, float colorBurst) {
    //ph_info phaseInfo;
    
    // Normalize the color burst phase (similar to the original code)
    float c = colorBurst / (2 * M_PI);
    config.phaseInfo[0] = c - std::floor(c);
    
    // Set phase alternation (in original code this is a boolean array, 
    // but we simplify to alternate based on scanline)
    config.phaseInfo[1] = 0.0f; //(scanlineY % 2) ? 1.0f : -0.0f;
}

void setupConfig() {
    // TODO: analyze and document - in apple ii land the colorburst is defined as negative 33 degrees,
    // But that shifts the colorburst the wrong way. We needed to bring it counterclockwise around the 
    // colorwheel.
    //float colorBurst = 2.0 * M_PI * (-33.0 / 360.0 + (imageLeft % 4) / 4.0);
    float colorBurst = (2.0 * M_PI * (33.0 / 360.0 )); // plus 0.25, 0.5 0.75, if imageLeft % 4 is 1 2 3
    float subcarrier = NTSC_FSC / NTSC_4FSC; /* 0.25, basically constant here */
    printf("ColorBurst: %.6f\n", colorBurst);
    printf("Subcarrier: %.6f\n", subcarrier);

    //config.phaseInfo = generatePhaseInfo(/* scanlineY */ 0, colorBurst);
    printf("Phase Info:\n");
    printf("Phase[0]: %.6f\n", config.phaseInfo[0]);
    printf("Phase[1]: %.6f\n", config.phaseInfo[1]);

    config.width = FRAME_WIDTH;
    config.height = FRAME_HEIGHT;
    config.colorBurst = colorBurst;
    config.subcarrier = subcarrier;
    config.videoSaturation = 1.0f;
    config.videoHue = 0.05f;
    config.videoBrightness = 0.0f;

   /* config = {
        .width = FRAME_WIDTH,
        .height = FRAME_HEIGHT,
        .colorBurst = colorBurst,
        .subcarrier = subcarrier,
        .videoSaturation = 1.0f,
        .videoHue = 0.05f,
        .videoBrightness = 0.0f,
            // Set up filter coefficients based on measured YIQ filter response
        .filterCoefficients = {
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f },
            { 0.0f, 0.0f, 0.0f }
        },
        .decoderOffset = {0.0f, 0.0f, 0.0f}, // TODO: first value should brightness. -1 to +1.
        .videoSaturation = 1.0f,
        .videoHue = 0.05f,
        .videoBrightness = 0.0f
    }; */
    
    generatePhaseInfo(/* scanlineY */ 0, colorBurst);

    // pre-calculate the sin/cos values for each of the 4 pixel positions (phases) based on
    // phaseInfo[0] and subcarrier.
    for (int ph = 0; ph < 4; ph++) {
        config.phase_sin[ph] = sinf(2.0f * M_PI * ((config.subcarrier * (ph % 4)) + config.phaseInfo[0]));
        config.phase_cos[ph] = cosf(2.0f * M_PI * ((config.subcarrier * (ph % 4)) + config.phaseInfo[0]));
        printf("phase_sin[%d]: %.6f, phase_cos[%d]: %.6f\n", ph, config.phase_sin[ph], ph, config.phase_cos[ph]);
    }

    // 4 phases
    // pixelYUV[2][4][3]
    // 2 input pixel values (0, 1)
    // 4 phases (0, 1, 2, 3)
    // 3 YUV components (Y, U, V)

    for (int ph = 0; ph < 4; ph++) {
        config.pixelYUV[0][ph][0] = 0.0f;
        config.pixelYUV[0][ph][1] = 0.0f;
        config.pixelYUV[0][ph][2] = 0.0f;

        config.pixelYUV[1][ph][0] = 1.0f;
        config.pixelYUV[1][ph][1] = config.phase_sin[ph];
        config.pixelYUV[1][ph][2] = config.phase_cos[ph];
        printf("pixelYUV[1][%d][0]: %.6f, pixelYUV[1][%d][1]: %.6f, pixelYUV[1][%d][2]: %.6f\n", ph, config.pixelYUV[1][ph][0], ph, config.pixelYUV[1][ph][1], ph, config.pixelYUV[1][ph][2]);
        printf("pixelYUV[0][%d][0]: %.6f, pixelYUV[0][%d][1]: %.6f, pixelYUV[0][%d][2]: %.6f\n", ph, config.pixelYUV[0][ph][0], ph, config.pixelYUV[0][ph][1], ph, config.pixelYUV[0][ph][2]);
    }
}

// Process a single scanline of Apple II video data
RGBA_t DisplayComposite::ntsc_lut_color(uint32_t inputBits, int pixelposition) 
{
    // First pass: Convert input to YIQ representation
    //float *yiq = &yiqBuffer[(pixelposition - NUM_TAPS) * 3];

    float oy = 0.0f;
    float oi = 0.0f;
    float oq = 0.0f;

    //for (int x = pixelposition - NUM_TAPS; x <= pixelposition + NUM_TAPS; x++) 
    for (int offset = - NUM_TAPS; offset <= NUM_TAPS; offset++)
    {
        int x = pixelposition + offset;
        int coeffIdx = std::abs(offset);
        bool bit = (inputBits & 1); // this might be backwards.
        float y = config.pixelYUV[bit][x % 4][0];
        float i = config.pixelYUV[bit][x % 4][1];
        float q = config.pixelYUV[bit][x % 4][2];

        oy += y * config.filterCoefficients[coeffIdx][0];  // Y
        oi += i * config.filterCoefficients[coeffIdx][1];  // I
        oq += q * config.filterCoefficients[coeffIdx][2];  // Q

        inputBits >>= 1; // this might be backwards.
    }
 
    // Convert pixel to RGB
    // Apply matrix and offset (matrix is 3x3, stored in row-major format)
    float r = config.decoderMatrix[0] * oy + config.decoderMatrix[1] * oi + config.decoderMatrix[2] * oq /* + offset[0] */;
    float g = config.decoderMatrix[3] * oy + config.decoderMatrix[4] * oi + config.decoderMatrix[5] * oq /* + offset[1] */;
    float b = config.decoderMatrix[6] * oy + config.decoderMatrix[7] * oi + config.decoderMatrix[8] * oq /* + offset[2] */;
    
    RGBA_t emit;
    // Clamp values and convert to 8-bit
    emit.r = std::clamp(static_cast<int>(r * 255), 0, 255);
    emit.g = std::clamp(static_cast<int>(g * 255), 0, 255);
    emit.b = std::clamp(static_cast<int>(b * 255), 0, 255);
    emit.a = 255;

    return emit;
}

/** 
 * extract the following into variables in the ntsc_config:
 * videoSaturation: 1.0f
 * videoHue: 0.05f
 * videoBrightness: 0.0f
 * whenever you change these values, re-call init_ntsc_lut().
 */
void DisplayComposite::init_ntsc_lut()
{
    // identity matrix?
    Matrix3x3 decoderMatrix(
        1, 0, 0,
        0, 1, 0,
        0, 0, 1);

    Matrix3x3 yiqMatrix(
        1, 0, 1.13983,
        1, -0.39465, -0.58060,
        1, 2.03206, 0
    );

    // saturation should be 0 to 1.0 
    // 0 is basically monochrome. transposing row/col here does not matter.
    //float videoSaturation = 1.0f;
    Matrix3x3 saturationMatrix(
        1, 0, 0,
        0, config.videoSaturation, 0,
        0, 0, config.videoSaturation
    );
    // if hue is 0, then hueMatrix becomes identity matrix. (transposing r
    float videoHue = 2 * (float)M_PI * config.videoHue;
    Matrix3x3 hueMatrix(
        1, 0, 0,
        0, cosf(videoHue), -sinf(videoHue),
        0, sinf(videoHue), cosf(videoHue)
    );

    decoderMatrix.multiply(saturationMatrix);
    decoderMatrix.multiply(hueMatrix);
    decoderMatrix.multiply(yiqMatrix);
    decoderMatrix.print();
    config.decoderMatrix = decoderMatrix;

    for (int phaseCount = 0; phaseCount < 4; phaseCount++)
    {
        for (uint32_t bitCount = 0; bitCount < (1 << ((NUM_TAPS * 2) + 1)); bitCount++)
        {
            //  set the bit pattern on either size of the center (16 + phaseCount)
            int center = (16 + phaseCount);

            RGBA_t rval = ntsc_lut_color(bitCount, center);
            ntsc_lut[phaseCount][bitCount] = rval;  //  Pull out the center pixel and save it.
        }
    }
}

DisplayComposite::DisplayComposite(computer_t * computer) : Display(computer)
{
    setupConfig();
    generate_filters(NUM_TAPS);

    uint64_t start = SDL_GetTicksNS();
    init_ntsc_lut();
    uint64_t end = SDL_GetTicksNS();
    printf("init_ntsc_lut() took %g microseconds\n", (end - start) / 1000.0);
}

bool DisplayComposite::update_display(cpu_state *cpu)
{
    static RGBA_t p_white = {.a=0xFF, .b=0xFF, .g=0xFF, .r=0xFF};
    static RGBA_t p_black = {.a=0xFF, .b=0x00, .g=0x00, .r=0x00};

    VideoScannerII *video_scanner = cpu->get_video_scanner();
    kill_color = true;

    uint8_t  video_idx;
    uint16_t video_bits;
    uint16_t rawbits = 0;
    uint32_t ntscidx = 0;
    uint32_t vcount = 0;
    uint32_t hcount = 0;
    uint32_t phase = 0;
    uint32_t count = 0;

    RGBA_t * output = buffer;
    uint8_t * video_data = video_scanner->get_video_data();
    int video_data_size = video_scanner->get_video_data_size();
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
            break;

        case VM_LORES:
        output_lores:
            kill_color = false;
            video_idx = video_byte;
            video_idx = (video_idx >> (vcount & 4)) & 0x0F;  // hi or lo nibble
            video_idx = (video_idx << 1) | (hcount & 1); // even/odd columns
            video_bits = lgr_bits[video_idx];
            break;

        case VM_HIRES:
        output_hires:
            kill_color = false;
            video_idx = video_byte;
            video_bits = hgr_bits[video_idx];
            break;
        }

        rawbits = rawbits | video_bits; // carryover from HGR shifted bytes

        if (kill_color) {
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
        }
        else {
            count = 14;

            // initialize NTSC LUT index at the beginning of each scan line
            if (hcount == 0) {
                for (int i = NUM_TAPS; i; --i)
                {
                    ntscidx = ntscidx >> 1;
                    if (rawbits & 1)
                        ntscidx = ntscidx | (1 << ((NUM_TAPS*2)));
                    rawbits = rawbits >> 1;
                    --count;
                }
            }

            // this section produces the indices for the NTSC LUT, uses them
            // to grab RGB pixels, and stores the RGB pixels in the output texture
            for ( ; count; --count)
            {
                ntscidx = ntscidx >> 1;
                if (rawbits & 1)
                    ntscidx = ntscidx | (1 << ((NUM_TAPS*2)));
                rawbits = rawbits >> 1;

                //  Use the phase and the bits as the index
                *output++ = ntsc_lut[phase][ntscidx];
                phase = (phase+1) & 3;
            }

            if (++hcount == 40) {
                // output trailing pixels at the end of each line
                for (int count = NUM_TAPS; count; --count) {
                    ntscidx = ntscidx >> 1;
                    *output++ = ntsc_lut[phase][ntscidx];
                    phase = (phase+1) & 3;
                }

                // reset values for the beginning of the next line
                hcount = 0;
                rawbits = 0;
                ntscidx = 0;

                ++vcount;
            }
        }
    }

    return Display::update_display(cpu);
}


void init_mb_display_composite(computer_t *computer, SlotType_t slot) {
    // alloc and init Display
    DisplayComposite *ds = new DisplayComposite(computer);

    computer->sys_event->registerHandler(SDL_EVENT_KEY_DOWN, [ds](const SDL_Event &event) {
        return handle_display_event(ds, event);
    });

    computer->register_shutdown_handler([ds]() {
        SDL_DestroyTexture(ds->get_texture());
        delete ds;
        return true;
    });

    computer->video_system->register_display(0, ds);
}
