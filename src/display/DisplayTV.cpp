
#include "display/DisplayTV.hpp"
#include "Device_ID.hpp"

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
RGBA_t DisplayTV::ntsc_lut_color(uint32_t inputBits, int pixelposition) 
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
void DisplayTV::init_ntsc_lut()
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

DisplayTV::DisplayTV(computer_t * computer) : DisplayComposite(computer)
{
    setupConfig();
    generate_filters(NUM_TAPS);

    uint64_t start = SDL_GetTicksNS();
    init_ntsc_lut();
    uint64_t end = SDL_GetTicksNS();
    printf("init_ntsc_lut() took %g microseconds\n", (end - start) / 1000.0);
}

bool DisplayTV::update_display(cpu_state *cpu)
{
    uint32_t ntscidx = 0;
    uint32_t phase = 0;

    output = buffer;
    begin_video_bits(cpu);
    if (video_data_size > 0) {
        for (vcount = 0; vcount < 192; ++vcount)
        {
            phase = 1;
            build_scanline(cpu, vcount);

            if (kill_color()) {
                for (int n = 0; n < 81; ++n) {
                    uint8_t video_bits = scanline[n];
                    for (int i = 7; i; --i) {
                        if (video_bits & 1)
                            *output++ = p_white;
                        else
                            *output++ = p_black;
                        video_bits >>= 1; 
                    }
                }
            }
            else {
                ntscidx = 0;

                // For now I am relying on the concidence that NUM_TAPS == 7
                // and # of signal bits in one byte of the scanline also == 7
                uint8_t video_bits = scanline[0];
                for (int i = NUM_TAPS; i; --i)
                {
                    ntscidx = ntscidx >> 1;
                    if (video_bits & 1)
                        ntscidx = ntscidx | (1 << ((NUM_TAPS*2)));
                    video_bits = video_bits >> 1;
                }

                for (int n = 1; n < 81; ++n) {
                    uint8_t video_bits = scanline[n];
                    for (int i = 7; i; --i) {
                        ntscidx = ntscidx >> 1;
                        if (video_bits & 1)
                            ntscidx = ntscidx | (1 << ((NUM_TAPS*2)));
                        video_bits = video_bits >> 1;

                        //  Use the phase and the bits as the index
                        *output++ = ntsc_lut[phase][ntscidx];
                        phase = (phase+1) & 3;
                    }
                }

                // output trailing pixels at the end of each line
                for (int count = NUM_TAPS; count; --count) {
                    ntscidx = ntscidx >> 1;
                    *output++ = ntsc_lut[phase][ntscidx];
                    phase = (phase+1) & 3;
                }
            }
        }
    }

    return Display::update_display(cpu);
}


void init_mb_display_tv(computer_t *computer, SlotType_t slot) {
    // alloc and init Display
    DisplayTV *ds = new DisplayTV(computer);
    ds->register_display_device(computer, DEVICE_ID_DISPLAY_TV);
}
