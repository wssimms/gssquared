#pragma once

#include <cstdint>
#include <cstdio>

template<typename bs_t, uint16_t HEIGHT, uint16_t WIDTH>
class Frame {
private:
    uint16_t hloc;
    uint16_t scanline;
    uint16_t f_width; // purely informational, for consumers
    uint16_t f_height;
    alignas(64) bs_t stream[HEIGHT][WIDTH];

public:
    Frame(uint16_t width, uint16_t height);  // pixels
    ~Frame() = default;
    void print();

    inline bs_t *data() {
        return stream[0];
    }

    inline void push(bs_t bit) { 
        stream[scanline][hloc++] = bit;
    }

    inline bs_t pull() { 
        return stream[scanline][hloc++];
    }
    
    inline void set_line(int line) { 
        scanline = line;
        hloc = 0;
    }

    void clear();
    
    // Getters for template parameters
    static constexpr uint16_t max_width() { return WIDTH; }
    static constexpr uint16_t max_height() { return HEIGHT; }
};

// Template implementation
template<typename bs_t, uint16_t HEIGHT, uint16_t WIDTH>
Frame<bs_t, HEIGHT, WIDTH>::Frame(uint16_t width, uint16_t height) {
    f_width = width;
    f_height = height;
    scanline = 0;
    hloc = 0;
}

template<typename bs_t, uint16_t HEIGHT, uint16_t WIDTH>
void Frame<bs_t, HEIGHT, WIDTH>::clear() {
    // Implementation would go here - clearing the bitstream array
    for (int i = 0; i < HEIGHT; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            stream[i][j] = bs_t{};
        }
    }
    scanline = 0;
    hloc = 0;
}

template<typename bs_t, uint16_t HEIGHT, uint16_t WIDTH>
void Frame<bs_t, HEIGHT, WIDTH>::print() {
    printf("Frame: %u x %u\n", WIDTH, HEIGHT);
    for (int i = 0; i < HEIGHT; i++) {
        set_line(i);
        uint16_t linepos = 0;
        for (int j = 0; j < WIDTH / 64; j++) {
            uint64_t wrd = 0;
            for (int b = 0; b < 64; b++) {
                wrd = (wrd << 1) | pull();
                if (linepos++ >= WIDTH) break; // don't go past the end of the line
            }
            printf("%016llx ", wrd);
            if (linepos >= WIDTH) break; // don't go past the end of the line
        }
        printf("\n");
    }
}