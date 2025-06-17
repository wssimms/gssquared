#include <SDL3/SDL.h>
#include "devices/displaypp/frame/frame_bit.hpp"
#include "devices/displaypp/frame/frame_byte.hpp"

int main(int argc, char **argv) {
    int testiterations = 10000;

    printf("Testing bitstream\n");

    uint64_t start = SDL_GetTicksNS();
    Frame_Bitstream frame(640, 200);
   
    for (int numframes = 0; numframes < testiterations; numframes++) {
        frame.clear();
        for (int i = 0; i < 200; i++) {
            frame.write_line(i);
            for (int j = 0; j < 320; j++) {
                frame.push(1);
                frame.push(0);
            }
            frame.close_line();
        }

    }
    uint64_t end = SDL_GetTicksNS();
    frame.print();
    printf("Write Time taken: %llu microseconds per frame\n", (end - start) / 1000 / testiterations);

    start = SDL_GetTicksNS();

    for (int numframes = 0; numframes < testiterations; numframes++) {
        for (int i = 0; i < 200; i++) {
            frame.read_line(i);
            for (int i = 0; i < 320; i++) {
                frame.pull();
                frame.pull();
            }
        }
    }
    end = SDL_GetTicksNS();
    printf("read Time taken: %llu microseconds per frame\n", (end - start) / 1000 / testiterations);

    printf("Testing bytestream\n");
    Frame_Bytestream frame_byte(640, 200);
    start = SDL_GetTicksNS();
    for (int numframes = 0; numframes < testiterations; numframes++) {
        for (int i = 0; i < 200; i++) {
            frame_byte.write_line(i);
            for (int j = 0; j < 320; j++) {
                frame_byte.push(1);
                frame_byte.push(0);
            }
        }
    }
    end = SDL_GetTicksNS();
    printf("Write Time taken: %llu microseconds per frame\n", (end - start) / 1000 / testiterations);

    start = SDL_GetTicksNS();
    int c = 0;
    for (int numframes = 0; numframes < testiterations; numframes++) {
        for (int i = 0; i < 200; i++) {
            frame_byte.read_line(i);
            for (int j = 0; j < 320; j++) {
                c += frame_byte.pull();
                c += frame_byte.pull();
            }
        }
    }
    end = SDL_GetTicksNS();
    printf("c: %d\n", c);
    printf("read Time taken: %llu microseconds per frame\n", (end - start) / 1000 / testiterations);
    printf("Size of bytestream: %d bytes\n", sizeof(frame_byte));
    return 0;
}
