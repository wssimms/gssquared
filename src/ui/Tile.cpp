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

#include "Tile.hpp"

Tile_t::Tile_t(const Style_t& initial_style) : style(initial_style) {}

/* Tile_t::~Tile_t() = default; */
    
    /**
     * @brief Sets the size of the content area. The actual tile size will include padding and borders.
     * @param content_width Width of the content area
     * @param content_height Height of the content area
     */
    void Tile_t::set_size(float content_width, float content_height) {
        content_w = content_width;
        content_h = content_height;
        // Total size includes padding on both sides and border width on both sides
        w = content_width + (style.padding * 2) + (style.border_width * 2);
        h = content_height + (style.padding * 2) + (style.border_width * 2);
    }

    /**
     * @brief Gets the content area position (accounting for padding and border)
     * @param out_x Output parameter for content x position
     * @param out_y Output parameter for content y position
     */
    void Tile_t::get_content_position(float* out_x, float* out_y) const {
        *out_x = x + style.border_width + style.padding;
        *out_y = y + style.border_width + style.padding;
    }

    /**
     * @brief Gets the content area size
     * @param out_w Output parameter for content width
     * @param out_h Output parameter for content height
     */
    void Tile_t::get_content_size(float* out_w, float* out_h) const {
        *out_w = content_w;
        *out_h = content_h;
    }

    /**
     * @brief Gets the total tile size including padding and borders
     * @param out_w Output parameter for total width
     * @param out_h Output parameter for total height
     */
    void Tile_t::get_size(float* out_w, float* out_h) const {
        *out_w = w;
        *out_h = h;
    }

    void Tile_t::set_padding(uint32_t new_padding) {
        style.padding = new_padding;
        // Recalculate total size based on new padding
        set_size(content_w, content_h);
    }

    void Tile_t::set_border_width(uint32_t width) {
        style.border_width = width;
        // Recalculate total size based on new border width
        set_size(content_w, content_h);
    }

    int Tile_t::calc_opacity(uint32_t color) {
        int opc = opacity < (color & 0xFF) ? opacity : (color & 0xFF);
        return(opc);
    }

    /**
     * @brief Renders the tile's background and border.
     * @param renderer The SDL renderer to use.
     */
        void Tile_t::render(SDL_Renderer* renderer) {
        if (!visible) return;

        uint32_t bcolor = (is_hovering) ? style.hover_color : style.border_color;

        // Draw background (includes padding area)
        SDL_SetRenderDrawColor(renderer, 
            (style.background_color >> 24) & 0xFF,
            (style.background_color >> 16) & 0xFF,
            (style.background_color >> 8) & 0xFF,
            calc_opacity(style.background_color)
        );
        
        SDL_FRect tile_rect = {x, y, w, h};
        SDL_RenderFillRect(renderer, &tile_rect);

        // Draw border if needed
        if (style.border_width > 0) {
            //printf("tile_t:render:hover: %d, draw border: %08X width: %d\n", (int)is_hovering,  bcolor, style.border_width);
            SDL_SetRenderDrawColor(renderer,
                (bcolor >> 24) & 0xFF,
                (bcolor >> 16) & 0xFF,
                (bcolor >> 8) & 0xFF,
                calc_opacity(bcolor)
            );
            for (uint32_t i = 0; i < style.border_width; i++) {
                SDL_FRect border_rect = {
                    x + i, y + i,
                    w - 2 * i, h - 2 * i
                };
                SDL_RenderRect(renderer, &border_rect);
            }
        }
    }
    
    bool Tile_t::handle_mouse_event(const SDL_Event& event) {
        if (!active || !visible) return(false);

        if (event.type == SDL_EVENT_MOUSE_MOTION) {
            float mouse_x = event.motion.x;
            float mouse_y = event.motion.y;
            
            // Check if mouse is within tile bounds
            bool is_inside = (mouse_x >= x && mouse_x <= x + w &&
                            mouse_y >= y && mouse_y <= y + h);
            
            // If we were hovering but mouse is now outside, or
            // we weren't hovering but mouse is now inside, trigger hover change
            if (is_hovering != is_inside) {
                is_hovering = is_inside;
                on_hover_changed(is_hovering);
            }
            // others may care about same event
        }
        else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && is_hovering) {
            on_click(event);
            return(true); // we did a thing.
        }
        else if (event.type == SDL_EVENT_WINDOW_MOUSE_LEAVE) {
            // Mouse left the window entirely, clear hover state
            if (is_hovering) {
                is_hovering = false;
                on_hover_changed(false);
            }
        }
        return(false);
    }
    
    bool Tile_t::is_visible() const { return visible; }
    bool Tile_t::is_active() const { return active; }
    bool Tile_t::is_mouse_hovering() const { return is_hovering; }
    void Tile_t::set_visible(bool v) { visible = v; }
    void Tile_t::set_active(bool a) { active = a; }
    void Tile_t::set_position(float new_x, float new_y) { x = new_x; y = new_y; }
    void Tile_t::set_background_color(uint32_t color) { style.background_color = color; }
    void Tile_t::set_border_color(uint32_t color) { style.border_color = color; }
    void Tile_t::set_hover_color(uint32_t color) { style.hover_color = color; }
    void Tile_t::set_click_callback(click_callback_t callback, void* data) {
        click_callback = callback;
        callback_data = data;
    }

    void Tile_t::set_click_callback(EventHandler handler) {
        click_callback_h = handler;
    }

    /**
     * @brief Called when the hover state changes.
     * @param hovering True if the mouse is now hovering, false otherwise.
     */
    void Tile_t::on_hover_changed(bool hovering) {
        //printf("tile:on_hover_changed: %d\n", hovering);
        is_hovering = hovering;
    }

    /**
     * @brief Called when the tile is clicked. (send the event for fullness, for text edit etc we need the exact x/y location)
     */
    void Tile_t::on_click(const SDL_Event& event) {
        if (click_callback) {
            click_callback(callback_data);
        }
        else if (click_callback_h) {
            click_callback_h(event);
        }
    }

    void Tile_t::set_text_renderer(TextRenderer *text_render) {
        this->text_render = text_render;
    }

    void Tile_t::set_opacity(int o) {
        opacity = o;
    }