#pragma once

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <string>

struct TextRenderer {
    /* Display Info */
    SDL_Window *window;
    SDL_Renderer *renderer;

    /* TTF Font Info */
    float font_size = 15.0f;
    TTF_Font *font = NULL;
    bool use_debug_font = false;
    TTF_TextEngine *engine = NULL;
    int font_line_height = 14;

    TextRenderer(SDL_Renderer *renderer, const std::string &font_path, float font_size);
    ~TextRenderer();
    void render(const std::string &text, int x, int y);
};

