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

#include "debug.hpp"
#include "cpu.hpp"
#include "devices/keyboard/keyboard.hpp"
#include "display/display.hpp"
#include "devices/game/mousewheel.hpp"
#include "devices/game/gamecontroller.hpp"
#include "devices/speaker/speaker.hpp"
#include "devices/loader.hpp"
#include "util/reset.hpp"
#include "devices/diskii/diskii.hpp"
#include "display/ntsc.hpp"
void handle_window_resize(cpu_state *cpu, int new_w, int new_h) {
    display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
        
    // Calculate new scale factors based on window size ratio
    float new_scale_x = (float)new_w / BASE_WIDTH;
    float new_scale_y = (float)new_h / (BASE_HEIGHT + ds->border_height*2);
 
    // TODO: technically this works, but, we should adjust the border_width to center the image.
    // Means borders should be in variable in the display_state_t.
    if (new_scale_x > (new_scale_y / 2.0f)) {
        new_scale_x = new_scale_y / 2.0f;
    }

    ds->border_width = ((new_w / new_scale_x)- BASE_WIDTH) / 2;

    printf("handle_window_resize: new_w: %d, new_h: %d new scale: %f, %f, border w: %d, h: %d\n", new_w, new_h, new_scale_x, new_scale_y, ds->border_width, ds->border_height);

    SDL_SetRenderScale(ds->renderer, new_scale_x, new_scale_y);
}

// Loops until there are no events in queue waiting to be read.
static SDL_Joystick *joystick = NULL;

bool handle_sdl_keydown(cpu_state *cpu, SDL_Event event) {

    // Ignore if only shift is pressed
    /* uint16_t mod = event.key.keysym.mod;
    SDL_Keycode key = event.key.keysym.sym; */
    SDL_Keymod mod = event.key.mod;
    SDL_Keycode key = event.key.key;

    if ((mod & SDL_KMOD_CTRL) && (key == SDLK_F10)) {
        if (mod & SDL_KMOD_ALT) {
            system_reset(cpu, true); 
        } else {
            system_reset(cpu, false); 
        }
        return true;
    }

    if (key == SDLK_F12) { 
        cpu->halt = HLT_USER; 
        return true;
    }
    if (key == SDLK_F9) { 
        toggle_clock_mode(cpu);
        return true; 
    }
    if (key == SDLK_F8) {
        //toggle_speaker_recording(cpu);
        debug_dump_disk_images(cpu);
        return true;
    }
    if (key == SDLK_F5) {
        flip_display_scale_mode(cpu);
        return true;
    }
    if ((key == SDLK_KP_PLUS || key == SDLK_KP_MINUS)) {
        printf("key: %x, mod: %x\n", key, mod);
        if (mod & SDL_KMOD_ALT) { // ALT == hue (windows key on my mac)
            config.videoHue += ((key == SDLK_KP_PLUS) ? 0.025f : -0.025f);
        } else if (mod & SDL_KMOD_SHIFT) { // WINDOWS == brightness
            config.videoSaturation += ((key == SDLK_KP_PLUS) ? 0.1f : -0.1f);
        }
        init_hgr_LUT();
        force_display_update(cpu);
        printf("video hue: %f, saturation: %f\n", config.videoHue, config.videoSaturation);
        return true;
    }
    /* if (key == SDLK_F7) {
        loader_execute(cpu);
        return true;
    } */
    if (key == SDLK_F6) {
        if (mod & SDL_KMOD_CTRL) {
            // dump hires image page 1
            display_dump_text_page(cpu, 1);
            return true;
        }
        if (mod & SDL_KMOD_SHIFT) {
            // dump hires image page 2
            display_dump_text_page(cpu, 2);
            return true;
        }
    }
    if (key == SDLK_F7) {
        if (mod & SDL_KMOD_CTRL) {
            // dump hires image page 1
            display_dump_hires_page(cpu, 1);
            return true;
        }
        if (mod & SDL_KMOD_SHIFT) {
            // dump hires image page 2
            display_dump_hires_page(cpu, 2);
            return true;
        }
    }
    if (key == SDLK_F3) {
        toggle_display_fullscreen(cpu);
        return true;
    }
    if (key == SDLK_F2) {
        toggle_display_engine(cpu);
        force_display_update(cpu);
        return true;
    }
    if (key == SDLK_F1) {
        display_capture_mouse(cpu, false);
        //SDL_SetWindowRelativeMouseMode(cpu->window, false);
        return true;
    }
    return false;

}


void event_poll(cpu_state *cpu, SDL_Event &event) {
    switch (event.type) {
        case SDL_EVENT_QUIT:
            if (DEBUG(DEBUG_GUI)) fprintf(stdout, "quit received, shutting down\n");
            cpu->halt = HLT_USER;
            break;

        case SDL_EVENT_WINDOW_RESIZED: {
            display_state_t *ds = (display_state_t *)get_module_state(cpu, MODULE_DISPLAY);
            if (ds && ds->window) {
                handle_window_resize(cpu, event.window.data1, event.window.data2);
            }
            break;
        }

        case SDL_EVENT_KEY_DOWN:
            if (handle_sdl_keydown(cpu, event)) break;
            handle_keydown_iiplus(cpu, event);
            break;

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            display_capture_mouse(cpu, true);
            //SDL_SetWindowRelativeMouseMode(cpu->window, true);
            break;

        case SDL_EVENT_MOUSE_WHEEL:
            handle_mouse_wheel(cpu, event.wheel.y);
            break;
        case SDL_EVENT_JOYSTICK_ADDED:
            /* this event is sent for each hotplugged stick, but also each already-connected joystick during SDL_Init(). */
            joystick_added(cpu, &event);
            break;
        case SDL_EVENT_JOYSTICK_REMOVED:
            joystick_removed(cpu, &event);
            break;
    }
}