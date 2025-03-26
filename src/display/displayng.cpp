
#include "display/ntsc.hpp"
#include "display/font.hpp"

uint8_t *frameBuffer;

void init_displayng() {
    // Create a 560x192 frame buffer
    frameBuffer = new uint8_t[560 * 192];

    buildHires40Font(MODEL_IIE, 0);
    setupConfig();
    init_hgr_LUT();
}