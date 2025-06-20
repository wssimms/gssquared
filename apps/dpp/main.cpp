#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <new>

#include <SDL3/SDL.h>

//#include "devices/displaypp/frame/frame_bit.hpp"
#include "devices/displaypp/frame/Frames.hpp"
#include "devices/displaypp/generate/AppleII.cpp"
#include "devices/displaypp/render/Monochrome560.hpp"
#include "devices/displaypp/render/NTSC560.hpp"
#include "devices/displaypp/CharRom.hpp"

int main(int argc, char **argv) {
    uint64_t start = 0, end = 0;

    //SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("DisplayPP Test Harness", 1120, 768, SDL_WINDOW_RESIZABLE);
    if (!window) {
        printf("Failed to create window\n");
        return 1;
    }
    SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        printf("Failed to create renderer\n");
        return 1;
    }
    //SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STREAMING, 560, 192);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, 560, 192);
    if (!texture) {
        printf("Failed to create texture\n");
        printf("SDL Error: %s\n", SDL_GetError());
        return 1;
    }
    if (!SDL_SetRenderVSync(renderer, SDL_RENDERER_VSYNC_DISABLED)) {
        printf("Failed to set render vsync\n");
        printf("SDL Error: %s\n", SDL_GetError());
        return 1;
    }

    const char *rname = SDL_GetRendererName(renderer);
    printf("Renderer: %s\n", rname);
    //SDL_SetHint(SDL_HINT_RENDER_VSYNC, "0");
    SDL_SetRenderScale(renderer, 2.0f, 4.0f);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_LINEAR);
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE); 
    int error = SDL_SetRenderTarget(renderer, nullptr);

    int testiterations = 10000;

#if 0
    // bitstream test. This performs worse than the bytestream.
    printf("Testing bitstream\n");

    start = SDL_GetTicksNS();
    alignas(64) Frame_Bitstream frame(640, 200);
   
    for (int numframes = 0; numframes < testiterations; numframes++) {
        frame.clear();
        for (int i = 0; i < FB_BITSTREAM_HEIGHT; i++) {
            frame.write_line(i);
            for (int j = 0; j < FB_BITSTREAM_WIDTH/2; j++) {
                frame.push(1);
                frame.push(0);
            }
            frame.close_line();
        }
    }

    end = SDL_GetTicksNS();
    frame.print();
    printf("Write Time taken: %llu microseconds per frame\n", (end - start) / 1000 / testiterations);

    start = SDL_GetTicksNS();

    for (int numframes = 0; numframes < testiterations; numframes++) {
        for (int i = 0; i < FB_BITSTREAM_HEIGHT; i++) {
            frame.read_line(i);
            for (int i = 0; i < FB_BITSTREAM_WIDTH/2; i++) {
                frame.pull();
                frame.pull();
            }
        }
    }
    end = SDL_GetTicksNS();
    printf("read Time taken: %llu microseconds per frame\n", (end - start) / 1000 / testiterations);

    printf("Testing bytestream\n");
#endif

    const uint16_t f_w = 560, f_h = 192;
    Frame560 *frame_byte = new(std::align_val_t(64)) Frame560(f_w, f_h);


    start = SDL_GetTicksNS();
    for (int numframes = 0; numframes < testiterations; numframes++) {
        for (int i = 0; i < 192; i++) {
            frame_byte->set_line(i);
            for (int j = 0; j < 560/2; j++) {
                frame_byte->push(1);
                frame_byte->push(0);
            }
        }
    }
    end = SDL_GetTicksNS();
    printf("Write Time taken: %llu ns per frame\n", (end - start) / testiterations);

    start = SDL_GetTicksNS();
    int c = 0;
    for (int numframes = 0; numframes < testiterations; numframes++) {
        for (int i = 0; i < f_h; i++) {
            frame_byte->set_line(i);
            for (int j = 0; j < f_w/2; j++) {
                c += frame_byte->pull();
                c += frame_byte->pull();
            }
        }
    }
    end = SDL_GetTicksNS();
    printf("read Time taken: %llu ns per frame\n", (end - start) / testiterations);
    //printf("Size of bytestream entries: %zu bytes\n", sizeof(bs_t));
    printf("c: %d\n", c);
    //frame_byte->print();

    uint8_t text_page[1024];
    uint8_t alt_text_page[1024];
    for (int i = 0; i < 1024; i++) {
        text_page[i] = i & 0xFF;
        alt_text_page[i] = (i+1) & 0xFF;
    }

    CharRom iiplus_rom("assets/roms/apple2_plus/char.rom");
    CharRom iie_rom("assets/roms/apple2e_enh/char.rom");

    if (!iiplus_rom.is_valid() || !iie_rom.is_valid()) {
        printf("Failed to load char roms\n");
        return 1;
    }

    Frame560RGBA *frame_rgba = new(std::align_val_t(64)) Frame560RGBA(f_w, f_h);

    Monochrome560 monochrome;
    NTSC560 ntsc_render;

    AppleII_Display display_iie(iie_rom);
    iie_rom.print_matrix(0x40);
    AppleII_Display display_iiplus(iiplus_rom);
    iiplus_rom.print_matrix(0x40);
    //display.set_char_set(true);
    start = SDL_GetTicksNS();
    for (int numframes = 0; numframes < testiterations; numframes++) {
        for (int l = 0; l < 24; l++) {
            display_iiplus.generate_text40(text_page, frame_byte, l);
        }
        monochrome.render(frame_byte, frame_rgba, (RGBA_t){.a = 0xFF, .b = 0x00, .g = 0xFF, .r = 0x00});
    }

    end = SDL_GetTicksNS();
    printf("text Time taken: %llu ns per frame\n", (end - start) / testiterations);

    SDL_FRect dstrect = {
        (float)0.0,
        (float)0.0,
        (float)560, 
        (float)192
    };
    SDL_FRect srcrect = {
        (float)0.0,
        (float)0.0,
        (float)560, 
        (float)192
    };

    int pitch;
    void *pixels;
    SDL_LockTexture(texture, NULL, &pixels, &pitch);
    std::memcpy(pixels, frame_rgba->data(), 560 * 192 * sizeof(RGBA_t));
    SDL_UnlockTexture(texture);

    uint64_t cumulative = 0;
    uint64_t times[900];
    uint64_t framecnt = 0;

    int generate_mode = 1;
    int render_mode = 1;
    int sharpness = 0;
    bool exiting = false;

    while (++framecnt && !exiting)  {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                exiting = true;
            }
            if (event.type == SDL_EVENT_KEY_DOWN) {
                if (event.key.key == SDLK_1) {
                    generate_mode = 1;
                }
                if (event.key.key == SDLK_2) {
                    generate_mode = 2;
                }
                if (event.key.key == SDLK_3) {
                    generate_mode = 3;
                }
                if (event.key.key == SDLK_4) {
                    generate_mode = 4;
                }
                if (event.key.key == SDLK_N) {
                    render_mode = 2;
                }
                if (event.key.key == SDLK_M) {
                    render_mode = 1;
                }
                if (event.key.key == SDLK_P) {
                    sharpness = 1 - sharpness;
                    SDL_SetTextureScaleMode(texture, sharpness ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
                }
            }
        }

        start = SDL_GetTicksNS();
      
        for (int l = 0; l < 24; l++) {
            switch (generate_mode) {
                case 1:
                    display_iiplus.generate_text40(text_page, frame_byte, l);
                    break;
                case 2:
                    display_iie.generate_text80(text_page, alt_text_page, frame_byte, l);
                    break;
                case 3:
                    display_iie.generate_lores40(text_page, frame_byte, l);
                    break;
                case 4:
                    display_iie.generate_lores80(text_page, alt_text_page, frame_byte, l);
                    break;        
            }
        }
        switch (render_mode) {
            case 1:
                monochrome.render(frame_byte, frame_rgba, (RGBA_t){.a = 0xFF, .b = 0x00, .g = 0xFF, .r = 0x00});
                break;
            case 2:
                ntsc_render.render(frame_byte, frame_rgba, (RGBA_t){.a = 0xFF, .b = 0x00, .g = 0xFF, .r = 0x00});
                break;
        }

        // update the texture - approx 300us
        SDL_LockTexture(texture, NULL, &pixels, &pitch);
        std::memcpy(pixels, frame_rgba->data(), 560 * 192 * sizeof(RGBA_t));
        SDL_UnlockTexture(texture);
        
        // update widnow - approx 300us
        SDL_RenderClear(renderer);
        SDL_RenderTexture(renderer, texture, &srcrect, &dstrect);       
        end = SDL_GetTicksNS();
        SDL_RenderPresent(renderer);      

        cumulative += (end-start);
        if (framecnt < 300) times[framecnt] = (end-start);
    }
    
    printf("Render Time taken:%llu  %llu ns per frame\n", cumulative, cumulative / 900);
    for (int i = 0; i < (framecnt > 300 ? 300 : framecnt); i++) {
        printf("%llu ", times[i]);
    }
    printf("\n");
    
    return 0;
}
