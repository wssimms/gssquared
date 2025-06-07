#pragma once

#include <SDL3/SDL.h>
#include "computer.hpp"
#include "util/EventQueue.hpp"
#include "display/types.hpp"
#include "ui/Clipboard.hpp"

#define BORDER_WIDTH 30
#define BORDER_HEIGHT 20


typedef enum {
    DISPLAY_WINDOWED_MODE = 0,
    DISPLAY_FULLSCREEN_MODE = 1,
    NUM_FULLSCREEN_MODES
} display_fullscreen_mode_t;

/** Display Modes */

typedef enum {
    DM_ENGINE_NTSC = 0,
    DM_ENGINE_RGB,
    DM_ENGINE_MONO,
    DM_NUM_COLOR_ENGINES
} display_color_engine_t;

typedef enum {
    DM_MONO_WHITE = 0,
    DM_MONO_GREEN,
    DM_MONO_AMBER,
    DM_NUM_MONO_MODES
} display_mono_color_t;

typedef enum {
    DM_PIXEL_FUZZ = 0,
    DM_PIXEL_SQUARE,
    DM_NUM_PIXEL_MODES
} display_pixel_mode_t;


struct video_system_t {
    SDL_Window *window; // primary emulated display window
    SDL_Renderer* renderer;

    display_fullscreen_mode_t display_fullscreen_mode = DISPLAY_WINDOWED_MODE;
    display_color_engine_t display_color_engine = DM_ENGINE_NTSC;
    display_mono_color_t display_mono_color = DM_MONO_GREEN;
    display_pixel_mode_t display_pixel_mode = DM_PIXEL_FUZZ;

    int border_width = BORDER_WIDTH;
    int border_height = BORDER_HEIGHT;
    float aspect_ratio = 0.0;

    EventQueue *event_queue = nullptr;

    bool force_full_frame_redraw = false;

    ClipboardImage *clip = nullptr;

    RGBA mono_color_table[DM_NUM_MONO_MODES] = {
        {0xFF, 0xFF, 0xFF, 0xFF }, // white
        {0xFF, 0x4A, 0xFF, 0x00 }, // green (was 55) chosen from measuring @ 549nm and https://academo.org/demos/wavelength-to-colour-relationship/
        {0xFF, 0x00, 0xBF, 0xFF }  // amber
    };
    uint32_t mono_color_table_u[DM_NUM_MONO_MODES] = {
        0xFFFFFFFF, // white
        0x00FF4AFF, // green (was 55) chosen from measuring @ 549nm and https://academo.org/demos/wavelength-to-colour-relationship/
        0xFFBF00FF, // amber
    };
    video_system_t(computer_t *computer);
    ~video_system_t();
    void window_resize(const SDL_Event &event);
    void toggle_fullscreen();
    void set_window_fullscreen(display_fullscreen_mode_t mode);
    display_fullscreen_mode_t get_window_fullscreen();
    void sync_window();
    void render_frame(SDL_Texture *texture);
    void clear();
    void present();
    void display_capture_mouse(bool capture);
    void raise();
    void raise(SDL_Window *window);
    void hide(SDL_Window *window);
    void show(SDL_Window *window);
    void set_full_frame_redraw();
    void send_engine_message();
    void toggle_display_engine();
    void set_display_engine(display_color_engine_t mode);
    void set_display_mono_color(display_mono_color_t mode);
    void flip_display_scale_mode();
    RGBA get_mono_color() { return mono_color_table[display_mono_color]; };
    uint32_t get_mono_color_u() { return mono_color_table_u[display_mono_color]; };
};
