#pragma once

#include <SDL3/SDL.h>
#include "computer.hpp"

#define BORDER_WIDTH 30
#define BORDER_HEIGHT 20



typedef enum {
    DISPLAY_WINDOWED_MODE = 0,
    DISPLAY_FULLSCREEN_MODE = 1,
    NUM_FULLSCREEN_MODES
} display_fullscreen_mode_t;


struct video_system_t {
    video_system_t(computer_t *computer);
    ~video_system_t();
    //void window_resize(int new_w, int new_h);
    void window_resize(const SDL_Event &event);
    void toggle_fullscreen();
    void render_frame(SDL_Texture *texture);
    void clear();
    void present();
    void display_capture_mouse(bool capture);
    void raise(SDL_Window *window);
    void hide(SDL_Window *window);
    void show(SDL_Window *window);

    SDL_Window *window;
    SDL_Renderer* renderer ;
    display_fullscreen_mode_t display_fullscreen_mode;

    int border_width = BORDER_WIDTH;
    int border_height = BORDER_HEIGHT;
};
