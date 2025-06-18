#include <cstdint>
#include <cstdio>
#include <cstring>

#include "frame_byte.hpp"

Frame_Bytestream::Frame_Bytestream(uint16_t width, uint16_t height) { // pixels
    f_width = width;
    f_height = height;
    scanline = 0;
    hloc = 0;
}

Frame_Bytestream::~Frame_Bytestream() {
  // nothing to do
}

void Frame_Bytestream::print() {
    // nope
    return;
    for (int i = 0; i < MAX_BITSTREAM_HEIGHT; i++) {
        for (int j = 0; j < MAX_BITSTREAM_WIDTH_WORDS; j++) {
            printf("%016llx ", display_bitstream[i][j]);
        }
        printf("\n");
    }
}

void Frame_Bytestream::clear() {
    // don't need this when we're bytewise
    ///memset(display_bitstream, 0, sizeof(display_bitstream));
}