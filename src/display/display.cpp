/*
 *   Copyright (c) 2025 Jawaid Bazyar

 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.

 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.

 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <SDL3/SDL.h>

#include "cpu.hpp"
//#include "gs2.hpp"
//#include "debug.hpp"

#include "display.hpp"
//#include "platforms.hpp"
#include "event_poll.hpp"
#include "display/filters.hpp"
#include "videosystem.hpp"
#include "util/EventDispatcher.hpp"

bool Display::update_display(cpu_state *cpu)
{
    flash_counter += 1;
    if (flash_counter == 30)
        flash_counter = 0;

    cpu->get_video_scanner()->end_video_cycle();

    void* pixels;
    int pitch;

    if (!SDL_LockTexture(screenTexture, NULL, &pixels, &pitch)) {
        fprintf(stderr, "Failed to lock texture: %s\n", SDL_GetError());
        return false;
    }
    printf("pixels:%p, this:%p, buffer:%p\n", pixels, this, buffer); fflush(stdout);
    memcpy(pixels, buffer, BASE_WIDTH * BASE_HEIGHT * sizeof(RGBA_t)); // load all buffer into texture
    SDL_UnlockTexture(screenTexture);

    video_system->render_frame(screenTexture);

    return true;
}

void Display::make_flipped() {
    for (int n = 0; n < 256; ++n) {
        uint8_t byte = (uint8_t)n;
        uint8_t flipped_byte = byte >> 7; // leave high bit as high bit
        for (int i = 7; i; --i) {
            flipped_byte = (flipped_byte << 1) | (byte & 1);
            byte >>= 1;
        }
        flipped[n] = flipped_byte;
    }
}

void Display::make_text40_bits() {
    for (uint16_t n = 0; n < 256; ++n) {
        uint16_t textbits = n;
        uint16_t video_bits = 0;
        for (int count = 7; count; --count) {
            video_bits = (video_bits << 1) | (textbits & 1);
            video_bits = (video_bits << 1) | (textbits & 1);
            textbits >>= 1;
        }
        text40_bits[n] = video_bits;
    }   
}

void Display::make_hgr_bits() {
    for (uint16_t n = 0; n < 256; ++n) {
        uint8_t  byte = flipped[n];
        uint16_t hgrbits = 0;
        for (int i = 7; i; --i) {
            hgrbits = (hgrbits << 1) | (byte & 1);
            hgrbits = (hgrbits << 1) | (byte & 1);
            byte >>= 1;
        }
        hgrbits = hgrbits << (byte & 1);
        hgr_bits[n] = hgrbits;
        //printf("%4.4x\n", hgrbits);
    }
}

void Display::make_lgr_bits() {
    for (int n = 0; n < 16; ++n) {

        uint8_t pattern = flipped[n];
        pattern >>= 3;

        //uint8_t pattern = (uint8_t)n;

        // form even column pattern
        uint16_t evnbits = 0;
        for (int i = 14; i; --i) {
            evnbits = (evnbits << 1) | (pattern & 1);
            pattern = ((pattern & 1) << 3) | (pattern >> 1); // rotate
        }

        // form odd column pattern
        uint16_t oddbits = 0;
        for (int i = 14; i; --i) {
            oddbits = (oddbits << 1) | (pattern & 1);
            pattern = ((pattern & 1) << 3) | (pattern >> 1); // rotate
        }

        lgr_bits[2*n]   = ((evnbits >> 2) | (oddbits << 12)) & 0x3FFF;
        lgr_bits[2*n+1] = ((oddbits >> 2) | (evnbits << 12)) & 0x3FFF;
    }
}

/**
 * Display Class Implementation
 */
Display::Display(computer_t * computer) {
    //debug_set_level(DEBUG_DISPLAY);

    make_flipped();
    make_hgr_bits();
    make_lgr_bits();
    make_text40_bits();

    this->computer = computer;
    event_queue = computer->event_queue;
    video_system = computer->video_system;

    flash_counter = 0;

    buffer = new RGBA_t[BASE_WIDTH * BASE_HEIGHT];
    memset(buffer, 0, BASE_WIDTH * BASE_HEIGHT * sizeof(RGBA_t));
    // TODO: maybe start it with apple logo?

    // Create the screen texture
    screenTexture = SDL_CreateTexture(video_system->renderer,
        PIXEL_FORMAT,
        SDL_TEXTUREACCESS_STREAMING,
        BASE_WIDTH, BASE_HEIGHT);

    if (!screenTexture) {
        fprintf(stderr, "Error creating screen texture: %s\n", SDL_GetError());
    }

    SDL_SetTextureBlendMode(screenTexture, SDL_BLENDMODE_NONE);
    // LINEAR gets us appropriately blurred pixels.
    // NEAREST gets us sharp pixels.
    SDL_SetTextureScaleMode(screenTexture, SDL_SCALEMODE_LINEAR);
}

Display::~Display() {
    delete[] buffer;
}

void Display::register_display_device(computer_t *computer, device_id id)
{
    computer->sys_event->registerHandler(SDL_EVENT_KEY_DOWN, [this](const SDL_Event &event) {
        return handle_display_event(this, event);
    });

    computer->register_shutdown_handler([this]() {
        SDL_DestroyTexture(this->get_texture());
        delete this;
        return true;
    });

    computer->video_system->register_display(id, this);
}

bool handle_display_event(Display *ds, const SDL_Event &event) {
    SDL_Keymod mod = event.key.mod;
    SDL_Keycode key = event.key.key;

    return false;

    if ((key == SDLK_KP_PLUS || key == SDLK_KP_MINUS)) {
        printf("key: %x, mod: %x\n", key, mod);
        if (mod & SDL_KMOD_ALT) { // ALT == hue (windows key on my mac)
            config.videoHue += ((key == SDLK_KP_PLUS) ? 0.025f : -0.025f);
            if (config.videoHue < -0.3f) config.videoHue = -0.3f;
            if (config.videoHue > 0.3f) config.videoHue = 0.3f;

        } else if (mod & SDL_KMOD_SHIFT) { // WINDOWS == brightness
            config.videoSaturation += ((key == SDLK_KP_PLUS) ? 0.1f : -0.1f);
            if (config.videoSaturation < 0.0f) config.videoSaturation = 0.0f;
            if (config.videoSaturation > 1.0f) config.videoSaturation = 1.0f;
        }
        //init_hgr_LUT();

        static char msgbuf[256];
        snprintf(msgbuf, sizeof(msgbuf), "Hue set to: %f, Saturation to: %f\n", config.videoHue, config.videoSaturation);
        ds->get_event_queue()->addEvent(new Event(EVENT_SHOW_MESSAGE, 0, msgbuf));
        return true;
    }
    return false;
}

/** Called by Clipboard to return current display buffer.
 * doubles scanlines and returns 2* the "native" height. */
 
void Display::get_buffer(uint8_t *buffer, uint32_t *width, uint32_t *height) {
    // pass back the size.
    *width = BASE_WIDTH;
    *height = BASE_HEIGHT * 2;
    // BMP files have the last scanline first. What? 
    // Copy RGB values without alpha channel
    RGBA_t *src = (RGBA_t *)buffer;
    uint8_t *dst = buffer;
    for (int scanline = BASE_HEIGHT - 1; scanline >= 0; scanline--) {
        for (int i = 0; i < BASE_WIDTH; i++) {
            *dst++ = src[scanline * BASE_WIDTH + i].b;
            *dst++ = src[scanline * BASE_WIDTH + i].g;
            *dst++ = src[scanline * BASE_WIDTH + i].r;
        }
        // do it again - scanline double
        for (int i = 0; i < BASE_WIDTH; i++) {
            *dst++ = src[scanline * BASE_WIDTH + i].b;
            *dst++ = src[scanline * BASE_WIDTH + i].g;
            *dst++ = src[scanline * BASE_WIDTH + i].r;
        }
    }
    static char msgbuf[256];
    snprintf(msgbuf, sizeof(msgbuf), "Screen snapshot taken");
    computer->event_queue->addEvent(new Event(EVENT_SHOW_MESSAGE, 0, msgbuf));
}

void display_dump_file(cpu_state *cpu, const char *filename, uint16_t base_addr, uint16_t sizer) {
    FILE *fp = fopen(filename, "wb");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open %s for writing\n", filename);
        return;
    }
    // Write 8192 bytes (0x2000) from memory starting at base_addr
    for (uint16_t offset = 0; offset < sizer; offset++) {
        uint8_t byte = cpu->mmu->read(base_addr + offset);
        fwrite(&byte, 1, 1, fp);
    }
    fclose(fp);
}

void display_dump_hires_page(cpu_state *cpu, int page) {
    uint16_t base_addr = (page == 1) ? 0x2000 : 0x4000;
    display_dump_file(cpu, "dump.hgr", base_addr, 0x2000);
    fprintf(stdout, "Dumped HGR page %d to dump.hgr\n", page);
}

void display_dump_text_page(cpu_state *cpu, int page) {
    uint16_t base_addr = (page == 1) ? 0x0400 : 0x0800;
    display_dump_file(cpu, "dump.txt", base_addr, 0x0400);
    fprintf(stdout, "Dumped TXT page %d to dump.txt\n", page);
}
