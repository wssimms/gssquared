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

#include <SDL3/SDL.h>
#include "Style.hpp"

/**
 * @brief Base class for UI elements that can be rendered and interacted with.
 * 
 * Tile_t provides core functionality for UI elements including:
 * - Positioning and sizing
 * - Background and border rendering
 * - Mouse interaction handling
 * - Click callback support
 */
class Tile_t {
protected:
    Style_t style;
    float x, y;                  // Position of the entire tile
    float w, h;                  // Total size including padding and border
    float content_w, content_h;  // Size of the content area
    bool visible = true;
    bool active = true;
    bool is_hovering = false;
    typedef void (*click_callback_t)(void* data);
    click_callback_t click_callback = nullptr;
    void* callback_data = nullptr;

public:
    /**
     * @brief Constructs a tile with the given style.
     * @param initial_style The initial style settings for the tile
     */
    Tile_t(const Style_t& initial_style = Style_t());
    
    /**
     * @brief Virtual destructor for proper cleanup of derived classes.
     */
    virtual ~Tile_t() = default;

    /**
     * @brief Sets the size of the content area.
     * @param content_width Width of the content area
     * @param content_height Height of the content area
     */
    void set_size(float content_width, float content_height);

    /**
     * @brief Gets the content area position.
     * @param out_x Output parameter for content x position
     * @param out_y Output parameter for content y position
     */
    void get_content_position(float* out_x, float* out_y) const;

    /**
     * @brief Gets the content area size.
     * @param out_w Output parameter for content width
     * @param out_h Output parameter for content height
     */
    void get_content_size(float* out_w, float* out_h) const;

    /**
     * @brief Gets the total tile size including padding and borders.
     * @param out_w Output parameter for total width
     * @param out_h Output parameter for total height
     */
    void get_size(float* out_w, float* out_h) const;

    /**
     * @brief Sets the padding around the content area.
     * @param new_padding The new padding value
     */
    void set_padding(uint32_t new_padding);

    /**
     * @brief Sets the border width.
     * @param width The new border width
     */
    void set_border_width(uint32_t width);

    /**
     * @brief Renders the tile.
     * @param renderer The SDL renderer to use
     */
    virtual void render(SDL_Renderer* renderer);

    /**
     * @brief Handles mouse events for the tile.
     * @param event The SDL event to handle
     */
    virtual void handle_mouse_event(const SDL_Event& event);

    // State getters
    bool is_visible() const;
    bool is_active() const;
    bool is_mouse_hovering() const;

    // State setters
    void set_visible(bool v);
    void set_active(bool a);
    void set_position(float new_x, float new_y);
    void set_background_color(uint32_t color);
    void set_border_color(uint32_t color);
    void set_hover_color(uint32_t color);
    void set_click_callback(click_callback_t callback, void* data = nullptr);

protected:
    /**
     * @brief Called when hover state changes.
     * @param hovering True if mouse is hovering, false otherwise
     */
    virtual void on_hover_changed(bool hovering);

    /**
     * @brief Called when tile is clicked.
     */
    virtual void on_click();
}; 