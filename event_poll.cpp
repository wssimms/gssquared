#include <SDL2/SDL.h>

#include "debug.hpp"
#include "cpu.hpp"
#include "devices/keyboard/keyboard.hpp"

// Loops until there are no events in queue waiting to be read.

void event_poll(cpu_state *cpu) {
    SDL_Event event;
    while(SDL_PollEvent(&event)) {
        switch (event.type) {
            case  SDL_QUIT:
                if (DEBUG(DEBUG_GUI)) fprintf(stdout, "quit received, shutting down\n");
                cpu->halt = HLT_USER;
                break;

            case SDL_KEYDOWN:
                handle_sdl_keydown(cpu, event);
                break;
        }
    }
}