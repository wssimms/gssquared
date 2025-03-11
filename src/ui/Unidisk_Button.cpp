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
#include "Unidisk_Button.hpp"
#include "MainAtlas.hpp"

/**
 * @brief A specialized button class for Unidisk drive interface.
 * 
 * This class extends Button_t to provide additional rendering capabilities
 * specific to Unidisk drive visualization. The base button rendering is preserved,
 * and additional visual elements can be drawn on top.
 */

void Unidisk_Button_t::set_key(uint64_t k) { key = k; }
uint64_t Unidisk_Button_t::get_key() const { return key; }

void Unidisk_Button_t::set_disk_status(drive_status_t statusx) {
    status = statusx;
}

    /**
 * @brief Renders the Unidisk button with additional drive-specific elements.
 * @param renderer The SDL renderer to use.
 */
void Unidisk_Button_t::render(SDL_Renderer* renderer) {
    this->set_assetID(Unidisk_Face);
/*     if (status.is_mounted)    
    else this->set_assetID(Unidisk_Face);
 */
    // First, perform the base button rendering
    Button_t::render(renderer);

    // Get content area position for additional rendering
    float content_x, content_y;
    get_content_position(&content_x, &content_y);

    // Additional rendering can be added here
    // This space intentionally left empty for manual implementation
    if ((key & 0xFF) == 0) aa->draw(Unidisk_Drive1, content_x + 11, content_y + 31);
    else aa->draw(Unidisk_Drive2, content_x + 11, content_y + 31);
 
    if (status.motor_on) aa->draw(Unidisk_Light, content_x + 2, content_y + 40);

    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0xFF);
    char text[32];
    snprintf(text, sizeof(text), "Slot %llu", (key >> 8));
    SDL_RenderDebugText(renderer, content_x + 62, content_y + 75, text);
    
    // TODO: if mounted and hovering, show the disk image name over the drive
}
