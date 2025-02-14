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

#include "Button.hpp"
#include "DiskII_Button.hpp"
#include "MainAtlas.hpp"

/**
 * @brief A specialized button class for DiskII drive interface.
 * 
 * This class extends Button_t to provide additional rendering capabilities
 * specific to DiskII drive visualization. The base button rendering is preserved,
 * and additional visual elements can be drawn on top.
 */

void DiskII_Button_t::set_key(uint64_t k) { key = k; }
uint64_t DiskII_Button_t::get_key() const { return key; }
void DiskII_Button_t::set_disk_running(bool running) { disk_running = running; }
bool DiskII_Button_t::get_disk_running() const { return disk_running; }
void DiskII_Button_t::set_disk_mounted(bool mounted) { disk_mounted = mounted; }
bool DiskII_Button_t::get_disk_mounted() const { return disk_mounted; }

    /**
 * @brief Renders the DiskII button with additional drive-specific elements.
 * @param renderer The SDL renderer to use.
 */
void DiskII_Button_t::render(SDL_Renderer* renderer) {
    if (disk_mounted) this->set_assetID(DiskII_Closed);
    else this->set_assetID(DiskII_Open);

        // First, perform the base button rendering
        Button_t::render(renderer);

        // Get content area position for additional rendering
        float content_x, content_y;
        get_content_position(&content_x, &content_y);

        // Additional rendering can be added here
        // This space intentionally left empty for manual implementation
        if ((key & 0xFF) == 1) aa->draw(DiskII_Drive1, content_x + 4, content_y + 4);
        else aa->draw(DiskII_Drive2, content_x + 4, content_y + 4);

        if (disk_running) aa->draw(DiskII_DriveLightOn, content_x + 30, content_y + 69);

        SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0xFF, 0xFF);
        char text[32];
        snprintf(text, sizeof(text), "Slot %d", (key >> 8));
        SDL_RenderDebugText(renderer, content_x + 62, content_y + 84, text);
        
        // TODO: if mounted and hovering, show the disk image name over the drive
    }
