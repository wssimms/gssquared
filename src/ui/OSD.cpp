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

#include <stdexcept>
#include <string>
#include <SDL3/SDL.h>
#include <SDL3/SDL_render.h>
#include <SDL3_image/SDL_image.h>

#include "cpu.hpp"
#include "DiskII_Button.hpp"
#include "Unidisk_Button.hpp"
#include "MousePositionTile.hpp"
#include "Container.hpp"
#include "AssetAtlas.hpp"
#include "Style.hpp"
#include "MainAtlas.hpp"
#include "OSD.hpp"
#include "display/display.hpp"
#include "util/mount.hpp"
#include "util/reset.hpp"

#define MOUSE_POSITION_TILE 0

// we need to use data passed to us, and pass it to the ShowOpenFileDialog, so when the file select event
// comes back later, we know which drive this was for.
// TODO: only allow one of these to be open at a time. If one is already open, disregard.

struct diskii_callback_data_t {
    OSD *osd;
    uint64_t key;
};

/** handle file dialog callback */
static void /* SDLCALL */ file_dialog_callback(void* userdata, const char* const* filelist, int filter)
{
    if (filelist[0] == nullptr) return; // user cancelled dialog

    diskii_callback_data_t *data = (diskii_callback_data_t *)userdata;

    OSD *osd = data->osd;

    // returns callback: /Users/bazyar/src/AppleIIDisks/33master.dsk when selecting
    // a disk image file.
    printf("file_dialog_callback: %s\n", filelist[0]);
    // 1. unmount current image (if present).
    // 2. mount new image.
    disk_mount_t dm;
    dm.filename = (char *)filelist[0];
    dm.slot = data->key >> 8;
    dm.drive = data->key & 0xFF;
    osd->cpu->mounts->unmount_media(dm);
    osd->cpu->mounts->mount_media(dm);
    SDL_RaiseWindow(osd->get_window());
}

void diskii_button_click(void *userdata) {
    diskii_callback_data_t *data = (diskii_callback_data_t *)userdata;
    OSD *osd = data->osd;

    static const SDL_DialogFileFilter filters[] = {
        { "Disk Images",  "do;po;woz;dsk;hdv;2mg" },
        { "All files",   "*" }
    };

    printf("diskii button clicked\n");
    SDL_ShowOpenFileDialog(file_dialog_callback, 
        userdata, 
        osd->get_window(),
        filters,
        sizeof(filters)/sizeof(SDL_DialogFileFilter),
        nullptr,
        false);
}

void unidisk_button_click(void *userdata) {
    diskii_callback_data_t *data = (diskii_callback_data_t *)userdata;
    OSD *osd = data->osd;

    static const SDL_DialogFileFilter filters[] = {
        { "Disk Images",  "po;dsk;hdv;2mg" },
        { "All files",   "*" }
    };

    printf("unidisk button clicked\n");
    SDL_ShowOpenFileDialog(file_dialog_callback, 
        userdata, 
        osd->get_window(),
        filters,
        sizeof(filters)/sizeof(SDL_DialogFileFilter),
        nullptr,
        false);
}

void set_color_display(void *data) {
    printf("set_color_display %p\n", data);
    cpu_state *cpu = (cpu_state *)data;
    set_display_color_mode(cpu, DM_COLOR_MODE);
}

void set_green_display(void *data) {
    printf("set_green_display %p\n", data);
    cpu_state *cpu = (cpu_state *)data;
    set_display_color_mode(cpu, DM_GREEN_MODE);
}

void set_amber_display(void *data) {
    printf("set_amber_display %p\n", data);
    cpu_state *cpu = (cpu_state *)data;
    set_display_color_mode(cpu, DM_AMBER_MODE);
}

void set_mhz_1_0(void *data) {
    printf("set_mhz_1_0 %p\n", data);
    cpu_state *cpu = (cpu_state *)data;
    set_clock_mode(cpu, CLOCK_1_024MHZ);
}

void set_mhz_2_8(void *data) {
    printf("set_mhz_2_8 %p\n", data);
    cpu_state *cpu = (cpu_state *)data;
    set_clock_mode(cpu, CLOCK_2_8MHZ);
}

void set_mhz_4_0(void *data) {
    printf("set_mhz_4_0 %p\n", data);
    cpu_state *cpu = (cpu_state *)data;
    set_clock_mode(cpu, CLOCK_4MHZ);
}

void set_mhz_infinity(void *data) {
    printf("set_mhz_infinity %p\n", data);
    cpu_state *cpu = (cpu_state *)data;
    set_clock_mode(cpu, CLOCK_FREE_RUN);
}

void click_reset_cpu(void *data) {
    printf("click_reset_cpu %p\n", data);
    cpu_state *cpu = (cpu_state *)data;
    system_reset(cpu, false);
}


/** -------------------------------------------------------------------------------------------------- */

SDL_Window* OSD::get_window() { 
    return window; 
}

OSD::OSD(cpu_state *cpu, SDL_Renderer *rendererp, SDL_Window *windowp, SlotManager_t *slot_manager, int window_width, int window_height) 
    : renderer(rendererp), window(windowp), window_w(window_width), window_h(window_height), cpu(cpu), slot_manager(slot_manager) {

    cpTexture = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGBA8888,
        SDL_TEXTUREACCESS_TARGET,
        window_w, 
        window_h
    );

    if (!cpTexture) {
        throw std::runtime_error(std::string("Error creating cpTexture: ") + SDL_GetError());
    }
    /* cpu->osd.controlPanelTexture = cpTexture; */

    SDL_SetRenderTarget(renderer, cpTexture);
    
    SDL_RenderClear(renderer);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    // make the background opaque and black.
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderFillRect(renderer, NULL);

    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xC0);
    SDL_FRect rect = {0, 50, (float)(window_w-100), (float)(window_h-100)};
    SDL_RenderFillRect(renderer, &rect);
    
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0xFF, 0xFF);
    SDL_RenderDebugText(renderer, 50, 80, "This is your menu. It isn't very done, hai!");

    Style_t CS;
    CS.padding = 4;
    CS.border_width = 2;
    CS.background_color = 0xFFFFFFFF;
    CS.border_color = 0x008000E0;
    CS.hover_color = 0x008080FF;
    Style_t DS;
    DS.background_color = 0x00000000;
    DS.border_color = 0xFFFFFFFF;
    DS.hover_color = 0x008080FF;
    DS.padding = 5;
    DS.border_width = 2;
    Style_t SC;
    SC.background_color = 0x800080FF;
    SC.border_color = 0xFFFFFFFF;
    SC.border_width = 2;
    SC.hover_color = 0x008080FF;
    SC.padding = 4;
    Style_t SS;
    SS.background_color = 0x0084C6FF;
    SS.border_color = 0xFFFFFFFF;
    SS.hover_color = 0x606060FF;
    SS.text_color = 0xFFFFFFFF;
    SS.padding = 4;
    SS.border_width = 1;

    aa = new AssetAtlas_t(renderer, (char *)"img/atlas.png");
    aa->set_elements(MainAtlas_count, asset_rects);

    // Create a container for our drive buttons
    Container_t *drive_container = new Container_t(renderer, 10, CS);  // Increased to 5 to accommodate the mouse position tile
    drive_container->set_position(600, 70);
    drive_container->set_size(415, 600);
    containers.push_back(drive_container);

    // TODO: create buttons based on what is in slots.
    // Create the buttons
    diskii_button1 = new DiskII_Button_t(aa, DiskII_Open, DS); // this needs to have our disk key . or alternately use a different callback.
    diskii_button1->set_key(0x600);
    diskii_button1->set_click_callback(diskii_button_click, new diskii_callback_data_t{this, 0x600});

    diskii_button2 = new DiskII_Button_t(aa, DiskII_Closed, DS);
    diskii_button2->set_key(0x601);
    diskii_button2->set_click_callback(diskii_button_click, new diskii_callback_data_t{this, 0x601});

    unidisk_button1 = new Unidisk_Button_t(aa, Unidisk_Face, DS); // this needs to have our disk key . or alternately use a different callback.
    unidisk_button1->set_key(0x500);
    unidisk_button1->set_click_callback(unidisk_button_click, new diskii_callback_data_t{this, 0x500});

    unidisk_button2 = new Unidisk_Button_t(aa, Unidisk_Face, DS); // this needs to have our disk key . or alternately use a different callback.
    unidisk_button2->set_key(0x501);
    unidisk_button2->set_click_callback(unidisk_button_click, new diskii_callback_data_t{this, 0x501});

    // Add buttons to container
    drive_container->add_tile(diskii_button1, 0);
    drive_container->add_tile(diskii_button2, 1);
    drive_container->add_tile(unidisk_button1, 2);
    drive_container->add_tile(unidisk_button2, 3);

#if MOUSE_POSITION_TILE
    mouse_pos = new MousePositionTile_t();
    mouse_pos->set_position(100,600) ;
    mouse_pos->set_size(150,20);
    mouse_pos->set_background_color(0xFFFFFFFF);  // White background
    mouse_pos->set_border_color(0x000000FF);      // Black border
    mouse_pos->set_border_width(1);
#endif

    // Initial layout
    drive_container->layout();

    // Create another container, this one for slots.
    Container_t *slot_container = new Container_t(renderer, 8, SC);  // Container for 8 slot buttons
    slot_container->set_position(100, 100);
    slot_container->set_size(320, 304);

    for (int i = 7; i >= 0; i--) {
        char slot_text[128];
        snprintf(slot_text, sizeof(slot_text), "Slot %d: %s", i, slot_manager->get_device(static_cast<SlotType_t>(i))->name);
        Button_t* slot = new Button_t(slot_text, SS);
        slot->set_size(300, 20);
        slot_container->add_tile(slot, 7 - i);    // Add in reverse order (7 to 0)
    }
    slot_container->layout();
    containers.push_back(slot_container);

    Container_t *mon_color_con = new Container_t(renderer, 3, SC);
    mon_color_con->set_position(100, 510);
    mon_color_con->set_size(200, 65);
    containers.push_back(mon_color_con);

    Style_t CB;
    CB.background_color = 0x00000000;
    CB.border_width = 1;
    CB.border_color = 0x000000FF;
    CB.padding = 2;
    Button_t *mc1 = new Button_t(aa, ColorDisplayButton, CB);
    Button_t *mc2 = new Button_t(aa, GreenDisplayButton, CB);
    Button_t *mc3 = new Button_t(aa, AmberDisplayButton, CB);
    mc1->set_click_callback(set_color_display, cpu);
    mc2->set_click_callback(set_green_display, cpu);
    mc3->set_click_callback(set_amber_display, cpu);
    mon_color_con->add_tile(mc1, 0);
    mon_color_con->add_tile(mc2, 1);
    mon_color_con->add_tile(mc3, 2);
    mon_color_con->layout();

    Container_t *speed_con = new Container_t(renderer, 4, SC);
    speed_con->set_position(100, 450);
    speed_con->set_size(260, 65);
    containers.push_back(speed_con);

    Button_t *sp1 = new Button_t(aa, MHz1_0Button, CB);
    Button_t *sp2 = new Button_t(aa, MHz2_8Button, CB);
    Button_t *sp3 = new Button_t(aa, MHz4_0Button, CB);
    Button_t *sp4 = new Button_t(aa, MHzInfinityButton, CB);
    sp1->set_click_callback(set_mhz_1_0, cpu);
    sp2->set_click_callback(set_mhz_2_8, cpu);
    sp3->set_click_callback(set_mhz_4_0, cpu);
    sp4->set_click_callback(set_mhz_infinity, cpu);
    speed_con->add_tile(sp1, 0);
    speed_con->add_tile(sp2, 1);
    speed_con->add_tile(sp3, 2);
    speed_con->add_tile(sp4, 3);
    speed_con->layout();
    speed_btn_10 = sp1;
    speed_btn_28 = sp2;
    speed_btn_40 = sp3;
    speed_btn_8 = sp4;

    Container_t *gen_con = new Container_t(renderer, 10, SC);
    gen_con->set_position(5, 100);
    gen_con->set_size(65, 300);
    Button_t *b1 = new Button_t(aa, ResetButton, CB);
    b1->set_click_callback(click_reset_cpu, cpu);
    gen_con->add_tile(b1, 0);
    gen_con->layout();
    containers.push_back(gen_con);
}

void OSD::update() {
    /** Control panel slide in/out logic */ 
    /* if control panel is sliding in, update position and acceleration */
    if (slideStatus == SLIDE_IN) {
        slidePosition+=slidePositionDelta;

        if (slidePosition > 0) {
            slidePosition = 0;
            slideStatus = SLIDE_NONE;
            currentSlideStatus = SLIDE_IN;
        }
        if (slidePositionDelta >= slidePositionDeltaMin) { // don't let it go to 0 or negative
            slidePositionDelta = slidePositionDelta - slidePositionAcceleration;
        }
    }

    /* if control panel is sliding out, update position and acceleration */
    if (slideStatus == SLIDE_OUT) {
        slidePosition-=slidePositionDelta;

        if (slidePosition < -slidePositionMax ) {
            slideStatus = SLIDE_NONE;
            currentSlideStatus = SLIDE_OUT;
            slidePosition = -slidePositionMax;
        }
        if (slidePositionDelta <= slidePositionDeltaMax) {
            slidePositionDelta = slidePositionDelta + slidePositionAcceleration;
        }
    }

    static int updCount=0;
    if (updCount++ > 60) {
        updCount = 0;
        cpu->mounts->dump();
    }

    // TODO: iterate over all drives based on what's in slots.
    // update disk status
    diskii_button1->set_disk_status(cpu->mounts->media_status(0x600));
    diskii_button2->set_disk_status(cpu->mounts->media_status(0x601));
    unidisk_button1->set_disk_status(cpu->mounts->media_status(0x500));
    unidisk_button2->set_disk_status(cpu->mounts->media_status(0x501));

    // background color update based on clock speed to highlight current button.
    speed_btn_10->set_background_color(0x000000FF);
    speed_btn_28->set_background_color(0x000000FF);
    speed_btn_40->set_background_color(0x000000FF);
    speed_btn_8->set_background_color(0x000000FF);
    if (cpu->clock_mode == CLOCK_1_024MHZ) {
        speed_btn_10->set_background_color(0x00FF00FF);
    } else if (cpu->clock_mode == CLOCK_2_8MHZ) {
        speed_btn_28->set_background_color(0x00FF00FF);
    } else if (cpu->clock_mode == CLOCK_4MHZ) {
        speed_btn_40->set_background_color(0x00FF00FF);
    } else if (cpu->clock_mode == CLOCK_FREE_RUN) {
        speed_btn_8->set_background_color(0x00FF00FF);
    }
}

/** Draw the control panel (if visible) */
void OSD::render() {
    if (currentSlideStatus == SLIDE_IN || (currentSlideStatus != slideStatus)) {
        float ox,oy;
        SDL_GetRenderScale(renderer, &ox, &oy);
        SDL_SetRenderScale(renderer, 1,1); // TODO: calculate these based on window size

        /** if current Status is out, don't draw. If status is in transition or IN, draw. */
        SDL_FRect cpTargetRect = {
            (float)slidePosition,
            (float)0, // no vertical offset
            (float)window_w+slidePosition, 
            (float)window_h
        };

        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        //printFRect(&cpTargetRect);
        
        // Render the container and its buttons into the cpTexture
        SDL_SetRenderTarget(renderer, cpTexture);
        for (Container_t* container : containers) {
            container->render();
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);

#if MOUSE_POSITION_TILE
        mouse_pos->render(renderer);
#endif

        SDL_SetRenderTarget(renderer, nullptr);

        // now render the cpTexture into window
        SDL_RenderTexture(renderer, cpTexture, NULL, &cpTargetRect);
        SDL_SetRenderScale(renderer, ox,oy);
    }
}

bool OSD::event(const SDL_Event &event) {
    bool active = (currentSlideStatus == SLIDE_IN);
    if (active) {
        // Let containers have a stab at the event
        for (Container_t* container : containers) {
            container->handle_mouse_event(event);
        }
        
#if MOUSE_POSITION_TILE 
        // call separately since not in a container. Want it to always get mouse events no matter what.
        mouse_pos->handle_mouse_event(event);
#endif
    }

    switch (event.type)
    {
        case SDL_EVENT_KEY_DOWN:
            printf("osd key down: %d %d %d\n", event.key.key, slideStatus, currentSlideStatus);
            if (event.key.key == SDLK_F4) {
                SDL_SetWindowRelativeMouseMode(window, false);
                // if we're already in motion, disregard this for now.
                if (!slideStatus) {
                    if (currentSlideStatus == SLIDE_IN) { // we are in right now, slide it out
                        slideStatus = SLIDE_OUT;   
                        slidePosition = 0;
                        slidePositionDelta = slidePositionDeltaMin;
                    } else if (currentSlideStatus == SLIDE_OUT) {
                        slideStatus = SLIDE_IN; // slide it in right to the top
                        slidePosition = -slidePositionMax;
                        slidePositionDelta = slidePositionDeltaMax;
                    }
                }
                return(true);
            }
            break;
    }    
    return(active);
}
