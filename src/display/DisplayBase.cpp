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

#include "DisplayBase.hpp"
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
    //printf("pixels:%p, this:%p, buffer:%p\n", pixels, this, buffer); fflush(stdout);
    memcpy(pixels, buffer, BASE_WIDTH * BASE_HEIGHT * sizeof(RGBA_t)); // load all buffer into texture
    SDL_UnlockTexture(screenTexture);

    video_system->render_frame(screenTexture, -7.0f);

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

#if 0
// Implement the switch, but text display doesn't use it yet.
void display_write_switches(void *context, uint16_t address, uint8_t value) {
    display_state_t *ds = (display_state_t *)context;
    switch (address) {
        case 0xC00E:
            ds->display_alt_charset = false;
            break;
        case 0xC00F:
            ds->display_alt_charset = true;
            break;
    }
}

uint8_t display_read_C01E(void *context, uint16_t address) {
    display_state_t *ds = (display_state_t *)context;
    return (ds->display_alt_charset) ? 0x80 : 0x00;
}

uint8_t display_read_C01F(void *context, uint16_t address) {
    display_state_t *ds = (display_state_t *)context;
    return (ds->f_80col) ? 0x80 : 0x00;
}

uint8_t display_read_C05EF(void *context, uint16_t address) {
    display_state_t *ds = (display_state_t *)context;
    ds->f_double_graphics = (address & 0x1); // this is inverted sense
    update_line_mode(ds);
    ds->video_system->set_full_frame_redraw();
    return 0;
}

void display_write_C05EF(void *context, uint16_t address, uint8_t value) {
    display_state_t *ds = (display_state_t *)context;
    ds->f_double_graphics = (address & 0x1); // this is inverted sense
    update_line_mode(ds);
    ds->video_system->set_full_frame_redraw();
}

void init_mb_device_display(computer_t *computer, SlotType_t slot) {
    cpu_state *cpu = computer->cpu;
    
    // alloc and init display state
    display_state_t *ds = new display_state_t;
    video_system_t *vs = computer->video_system;

    ds->video_system = vs;
    ds->event_queue = computer->event_queue;
    ds->computer = computer;
    MMU_II *mmu = computer->mmu;
    ds->mmu = mmu;

#if 1
    // new display engine setup
    CharRom *charrom = nullptr;
    if (computer->platform->id == PLATFORM_APPLE_IIE) {
        charrom = new CharRom("assets/roms/apple2e/char.rom");
    } else if (computer->platform->id == PLATFORM_APPLE_II_PLUS) {
        charrom = new CharRom("assets/roms/apple2_plus/char.rom");
    }
    ds->a2_display = new AppleII_Display(*charrom);
    uint16_t f_w = BASE_WIDTH+20;
    uint16_t f_h = BASE_HEIGHT;
    ds->frame_rgba = new(std::align_val_t(64)) Frame560RGBA(f_w, f_h);
    ds->frame_bits = new(std::align_val_t(64)) Frame560(f_w, f_h);
    ds->frame_rgba->clear(); // clear the frame buffers at startup.
    ds->frame_bits->clear();

    // Create the screen texture
    ds->screenTexture = SDL_CreateTexture(vs->renderer,
        PIXEL_FORMAT,
        SDL_TEXTUREACCESS_STREAMING,
        BASE_WIDTH+20, BASE_HEIGHT);

    if (!ds->screenTexture) {
        fprintf(stderr, "Error creating screen texture: %s\n", SDL_GetError());
    }

    SDL_SetTextureBlendMode(ds->screenTexture, SDL_BLENDMODE_NONE); /* GRRRRRRR. This was defaulting to SDL_BLENDMODE_BLEND. */
    // LINEAR gets us appropriately blurred pixels.
    // NEAREST gets us sharp pixels.
    SDL_SetTextureScaleMode(ds->screenTexture, SDL_SCALEMODE_LINEAR);

#else
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
#endif

    // set in CPU so we can reference later
    set_module_state(cpu, MODULE_DISPLAY, ds);
    
    mmu->set_C0XX_read_handler(0xC050, { txt_bus_read_C050, ds });
    mmu->set_C0XX_write_handler(0xC050, { txt_bus_write_C050, ds });
    mmu->set_C0XX_read_handler(0xC051, { txt_bus_read_C051, ds });
    mmu->set_C0XX_read_handler(0xC052, { txt_bus_read_C052, ds });
    mmu->set_C0XX_write_handler(0xC051, { txt_bus_write_C051, ds });
    mmu->set_C0XX_write_handler(0xC052, { txt_bus_write_C052, ds });
    mmu->set_C0XX_read_handler(0xC053, { txt_bus_read_C053, ds });
    mmu->set_C0XX_write_handler(0xC053, { txt_bus_write_C053, ds });
    mmu->set_C0XX_read_handler(0xC054, { txt_bus_read_C054, ds });
    mmu->set_C0XX_write_handler(0xC054, { txt_bus_write_C054, ds });
    mmu->set_C0XX_read_handler(0xC055, { txt_bus_read_C055, ds });
    mmu->set_C0XX_write_handler(0xC055, { txt_bus_write_C055, ds });
    mmu->set_C0XX_read_handler(0xC056, { txt_bus_read_C056, ds });
    mmu->set_C0XX_write_handler(0xC056, { txt_bus_write_C056, ds });
    mmu->set_C0XX_read_handler(0xC057, { txt_bus_read_C057, ds });
    mmu->set_C0XX_write_handler(0xC057, { txt_bus_write_C057, ds });

    for (int i = 0x04; i <= 0x0B; i++) {
        mmu->set_page_shadow(i, { txt_memory_write, cpu });
    }
    for (int i = 0x20; i <= 0x5F; i++) {
        mmu->set_page_shadow(i, { hgr_memory_write, cpu });
    }

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
        return update_display_apple2(cpu);
    });

    if (computer->platform->id == PLATFORM_APPLE_IIE) {
        ds->display_alt_charset = false;
        mmu->set_C0XX_write_handler(0xC00C, { ds_bus_write_C00X, ds });
        mmu->set_C0XX_write_handler(0xC00D, { ds_bus_write_C00X, ds });
        mmu->set_C0XX_write_handler(0xC00E, { display_write_switches, ds });
        mmu->set_C0XX_write_handler(0xC00F, { display_write_switches, ds });
        mmu->set_C0XX_read_handler(0xC01E, { display_read_C01E, ds });
    }
}
#endif

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
