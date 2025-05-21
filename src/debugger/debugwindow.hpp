#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include "cpu.hpp"
#include "util/TextRenderer.hpp"

struct debug_window_t {
    cpu_state *cpu;
    SDL_Window *window;
    SDL_Renderer *renderer;
    int window_width = 800;
    int window_height = 800;
    int window_margin = 5;
    int control_area_height = 100;
    int lines_in_view_area = 10;
    SDL_WindowID window_id;
    bool window_open = false;
    int view_position = 0;
    
    TextRenderer *text_renderer;
    int font_line_height = 14;

    void init(cpu_state *cpu);
    void render(cpu_state *cpu);
    bool handle_event(SDL_Event &event);
    bool is_open();
    void set_open();
    void set_closed();
    void resize(int width, int height);
    void separator_line(int y);
    void draw_text(int y, const char *text);
};
