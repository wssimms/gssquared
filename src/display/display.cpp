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
#include "gs2.hpp"
#include "debug.hpp"

#include "display.hpp"
#include "text_40x24.hpp"
#include "lores_40x48.hpp"
#include "hgr_280x192.hpp"
//#include "platforms.hpp"
#include "event_poll.hpp"
//#include "display/types.hpp"
#include "display/displayng.hpp"
#include "display/hgr.hpp"
#include "display/lgr.hpp"
#include "display/ntsc.hpp"
//#include "devices/videx/videx.hpp"
//#include "devices/videx/videx_80x24.hpp"
//#include "devices/annunciator/annunciator.hpp"
#include "videosystem.hpp"

/**
 * This is effectively a "redraw the entire screen each frame" method now.
 */
bool new_update_display_apple2(cpu_state *cpu) {
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
    video_system_t *vs = ds->video_system;

    static RGBA_t p_white = { 0xFF, 0xFF, 0xFF, 0xFF };

    switch (vs->display_color_engine) {
        case DM_ENGINE_NTSC:
            if (ds->kill_color) {
                newProcessAppleIIFrame_Mono(cpu, (RGBA_t *)(ds->buffer), p_white);
            }
            else {
                newProcessAppleIIFrame_NTSC(cpu, (RGBA_t *)(ds->buffer));
            }
            break;

        case DM_ENGINE_RGB:         
            newProcessAppleIIFrame_RGB(cpu, (RGBA_t *)(ds->buffer));               
            break;

        default:
            newProcessAppleIIFrame_Mono(cpu, (RGBA_t *)(ds->buffer), vs->get_mono_color());
            break;
    }

    void* pixels;
    int pitch;

    if (!SDL_LockTexture(ds->screenTexture, NULL, &pixels, &pitch)) {
        fprintf(stderr, "Failed to lock texture: %s\n", SDL_GetError());
        return false;
    }
    memcpy(pixels, ds->buffer, BASE_WIDTH * BASE_HEIGHT * sizeof(RGBA_t)); // load all buffer into texture
    SDL_UnlockTexture(ds->screenTexture);

    vs->render_frame(ds->screenTexture);

    return true;
}

void update_flash_state(cpu_state *cpu) {
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);

    // 2 times per second (every 30 frames), the state of flashing characters (those matching 0b01xxxxxx) must be reversed.
    
    if (++(ds->flash_counter) < 15) {
        return;
    }
    ds->flash_counter = 0;
    ds->flash_state = !ds->flash_state;
}

void display_state_t::make_flipped() {
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

void display_state_t::make_text40_bits() {
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

void display_state_t::make_hgr_bits() {
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

void display_state_t::make_lgr_bits() {
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
 * display_state_t Class Implementation
 */
display_state_t::display_state_t() {
    //debug_set_level(DEBUG_DISPLAY);

    make_flipped();
    make_hgr_bits();
    make_lgr_bits();
    make_text40_bits();

    flash_state = false;
    flash_counter = 0;

    kill_color = false;

    buffer = new uint8_t[BASE_WIDTH * BASE_HEIGHT * sizeof(RGBA_t)];
    memset(buffer, 0, BASE_WIDTH * BASE_HEIGHT * sizeof(RGBA_t)); // TODO: maybe start it with apple logo?
}

display_state_t::~display_state_t() {
    delete[] buffer;
}


bool handle_display_event(display_state_t *ds, const SDL_Event &event) {
    SDL_Keymod mod = event.key.mod;
    SDL_Keycode key = event.key.key;

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
        //force_display_update(ds);
        //ds->video_system->set_full_frame_redraw();
        static char msgbuf[256];
        snprintf(msgbuf, sizeof(msgbuf), "Hue set to: %f, Saturation to: %f\n", config.videoHue, config.videoSaturation);
        ds->event_queue->addEvent(new Event(EVENT_SHOW_MESSAGE, 0, msgbuf));
        return true;
    }
    return false;
}

/** Called by Clipboard to return current display buffer.
 * doubles scanlines and returns 2* the "native" height. */
 
void display_engine_get_buffer(computer_t *computer, uint8_t *buffer, uint32_t *width, uint32_t *height) {
    display_state_t *ds = (display_state_t *)get_module_state(computer->cpu, MODULE_DISPLAY);
    // pass back the size.
    *width = BASE_WIDTH;
    *height = BASE_HEIGHT * 2;
    // BMP files have the last scanline first. What? 
    // Copy RGB values without alpha channel
    RGBA_t *src = (RGBA_t *)ds->buffer;
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

void init_mb_device_display(computer_t *computer, SlotType_t slot) {
    cpu_state *cpu = computer->cpu;
    
    // alloc and init display state
    display_state_t *ds = new display_state_t;
    video_system_t *vs = computer->video_system;
    ds->video_system = vs;
    printf("ds->video_system:%p\n", ds->video_system); fflush(stdout);
    ds->event_queue = computer->event_queue;

    // Create the screen texture
    ds->screenTexture = SDL_CreateTexture(vs->renderer,
        PIXEL_FORMAT,
        SDL_TEXTUREACCESS_STREAMING,
        BASE_WIDTH, BASE_HEIGHT);

    if (!ds->screenTexture) {
        fprintf(stderr, "Error creating screen texture: %s\n", SDL_GetError());
    }

    SDL_SetTextureBlendMode(ds->screenTexture, SDL_BLENDMODE_NONE); /* GRRRRRRR. This was defaulting to SDL_BLENDMODE_BLEND. */
    // LINEAR gets us appropriately blurred pixels.
    // NEAREST gets us sharp pixels.
    SDL_SetTextureScaleMode(ds->screenTexture, SDL_SCALEMODE_LINEAR);

    init_displayng();

    // set in CPU so we can reference later
    set_module_state(cpu, MODULE_DISPLAY, ds);
    
    /*
    computer->mmu->set_C0XX_read_handler(0xC050, { txt_bus_read_C050, ds });
    computer->mmu->set_C0XX_write_handler(0xC050, { txt_bus_write_C050, ds });
    computer->mmu->set_C0XX_read_handler(0xC051, { txt_bus_read_C051, ds });
    computer->mmu->set_C0XX_write_handler(0xC051, { txt_bus_write_C051, ds });
    computer->mmu->set_C0XX_read_handler(0xC052, { txt_bus_read_C052, ds });
    computer->mmu->set_C0XX_write_handler(0xC052, { txt_bus_write_C052, ds });
    computer->mmu->set_C0XX_read_handler(0xC053, { txt_bus_read_C053, ds });
    computer->mmu->set_C0XX_write_handler(0xC053, { txt_bus_write_C053, ds });
    computer->mmu->set_C0XX_read_handler(0xC054, { txt_bus_read_C054, ds });
    computer->mmu->set_C0XX_write_handler(0xC054, { txt_bus_write_C054, ds });
    computer->mmu->set_C0XX_read_handler(0xC055, { txt_bus_read_C055, ds });
    computer->mmu->set_C0XX_write_handler(0xC055, { txt_bus_write_C055, ds });
    computer->mmu->set_C0XX_read_handler(0xC056, { txt_bus_read_C056, ds });
    computer->mmu->set_C0XX_write_handler(0xC056, { txt_bus_write_C056, ds });
    computer->mmu->set_C0XX_read_handler(0xC057, { txt_bus_read_C057, ds });
    computer->mmu->set_C0XX_write_handler(0xC057, { txt_bus_write_C057, ds });
    */
    /*
    for (int i = 0x04; i <= 0x0B; i++) {
        computer->mmu->set_page_shadow(i, { txt_memory_write, cpu });
    }
    for (int i = 0x20; i <= 0x5F; i++) {
        computer->mmu->set_page_shadow(i, { hgr_memory_write, cpu });
    }
    */

    computer->sys_event->registerHandler(SDL_EVENT_KEY_DOWN, [ds](const SDL_Event &event) {
        return handle_display_event(ds, event);
    });

    computer->register_shutdown_handler([ds]() {
        SDL_DestroyTexture(ds->screenTexture);
        deinit_displayng();
        delete ds;
        return true;
    });

    vs->register_frame_processor(0, [cpu]() -> bool {
        return new_update_display_apple2(cpu);
    });

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
