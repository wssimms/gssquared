
#include <cstdio>
#include "display/ntsc_setup.hpp"

ntsc_config config ;

#define FRAME_WIDTH 560   // necessary?
#define FRAME_HEIGHT 192

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
