#pragma once

#include <cstdint>

#define MAX_BITSTREAM_WIDTH 640
#define MAX_BITSTREAM_WIDTH_WORDS ((MAX_BITSTREAM_WIDTH / 64)+1)
#define MAX_BITSTREAM_HEIGHT 200

class Frame_Bitstream {
private:
    int f_width; // purely informational, for consumers
    int f_height;
    int scanline;
    uint64_t working;
    int hpos; // bit position in working
    int hloc;
    uint64_t display_bitstream[MAX_BITSTREAM_HEIGHT][MAX_BITSTREAM_WIDTH_WORDS];

public:
    Frame_Bitstream(uint16_t width, uint16_t height);  // pixels
    ~Frame_Bitstream();
    void print();

    inline void close_line() {
        working <<= (64 - hpos); // shift last bits into place for read
        display_bitstream[scanline][hloc] = working;
    };

    inline void push(bool bit) { 
        working = (working << 1) | bit;
        hpos++;
        if (hpos>=64) { 
            display_bitstream[scanline][hloc] = working; 
            hloc++;
            hpos = 0;
            working = 0;
        } 
    };

    inline bool pull() { 
        bool bit = (working & ((uint64_t)1 << 63)) == 1; 
        hpos++; 
        if (hpos>=64) { 
            working = display_bitstream[scanline][hloc];
            hloc++;
            hpos = 0;
        }
        return bit;
    };
    
    inline void read_line(int line) { 
        scanline = line; 
        hpos = 0;
        hloc = 1;
        working = display_bitstream[scanline][0]; // preload first word
    };

    inline void write_line(int line) { 
        scanline = line; 
        hpos = 0;
        hloc = 0;
        working = 0;
    };

    void clear();
};
