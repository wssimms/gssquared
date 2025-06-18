#pragma once

#include <cstdint>

#define MAX_BITSTREAM_WIDTH 640
#define MAX_BITSTREAM_WIDTH_WORDS ((MAX_BITSTREAM_WIDTH / 64)+1)
#define MAX_BITSTREAM_HEIGHT 200

typedef uint32_t bs_t;

class Frame_Bytestream {
private:
    int f_width; // purely informational, for consumers
    int f_height;
    int scanline;
    
    int hloc;
    bs_t display_bitstream[MAX_BITSTREAM_HEIGHT][MAX_BITSTREAM_WIDTH];

public:
    Frame_Bytestream(uint16_t width, uint16_t height);  // pixels
    ~Frame_Bytestream();
    void print();

    inline void push(bs_t bit) { 
        display_bitstream[scanline][hloc++] = bit;
    };

    inline bs_t pull() { 
        return display_bitstream[scanline][hloc++];
    };
    
    inline void read_line(int line) { 
        scanline = line; 
        hloc = 0;
    };

    inline void write_line(int line) { 
        scanline = line; 
        hloc = 0;
    };

    void clear();
};
