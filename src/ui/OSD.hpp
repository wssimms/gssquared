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
#pragma once

#include <vector>

#include <SDL3/SDL.h>

#include "cpu.hpp"
#include "slots.hpp"

#include "DiskII_Button.hpp"
#include "Unidisk_Button.hpp"
#include "Container.hpp"
#include "MousePositionTile.hpp"
#include "AssetAtlas.hpp"

#define SLIDE_IN 1
#define SLIDE_OUT 2
#define SLIDE_NONE 0

#define slidePositionDeltaMax 200
#define slidePositionDeltaMin 20
#define slidePositionAcceleration 10
#define slidePositionMax 1120

/**
 * @brief On-Screen Display manager class.
 * 
 * Handles the sliding control panel and its contained UI elements
 * including disk drive controls, slot buttons, and monitor controls.
 */
class OSD {
protected:
    int slideStatus = SLIDE_NONE;
    int currentSlideStatus = SLIDE_OUT;
    int slidePosition = -slidePositionMax;
    int slidePositionDelta = slidePositionDeltaMax;
    SlotManager_t *slot_manager = nullptr;
    
    DiskII_Button_t *diskii_button1 = nullptr;
    DiskII_Button_t *diskii_button2 = nullptr;
    Unidisk_Button_t *unidisk_button1 = nullptr;
    Unidisk_Button_t *unidisk_button2 = nullptr;
    Button_t *speed_btn_10 = nullptr;
    Button_t *speed_btn_28 = nullptr;
    Button_t *speed_btn_40 = nullptr;
    Button_t *speed_btn_8 = nullptr;

    std::vector<Container_t *> containers;

    MousePositionTile_t* mouse_pos = nullptr;
    AssetAtlas_t *aa = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *cpTexture = nullptr;
    SDL_Window *window = nullptr;
    int window_w = 0;
    int window_h = 0;
    bool raise_window_on_next_frame = false;

public:
    cpu_state *cpu = nullptr;
    

    /**
     * @brief Constructs the OSD with the given renderer and window.
     * 
     * @param rendererp SDL renderer to use
     * @param windowp SDL window to render to
     * @param window_width Width of the window
     * @param window_height Height of the window
     */
    OSD(cpu_state *cpu,SDL_Renderer *rendererp, SDL_Window *windowp, SlotManager_t *slot_manager, int window_width, int window_height);

    /**
     * @brief Gets the SDL window associated with this OSD.
     * @return Pointer to the SDL window
     */
    SDL_Window *get_window();

    /**
     * @brief Updates the OSD state, including slide animations.
     */
    void update();

    /**
     * @brief Renders the OSD and all its components.
     */
    void render();

    /**
     * @brief Handles SDL events for the OSD.
     * @param event The SDL event to process
     */
    bool event(const SDL_Event &event);

    void set_raise_window();
};