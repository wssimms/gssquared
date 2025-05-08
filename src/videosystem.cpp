
#include "gs2.hpp"
#include "videosystem.hpp"
#include "display/display.hpp"

video_system_t::video_system_t() {
    
    // TODO: this is to suggest opengl. metal backlogs periodically.
    //SDL_SetHint(SDL_HINT_RENDER_DRIVER, "opengl");

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Error initializing SDL: %s\n", SDL_GetError());
    }

    int window_width = (BASE_WIDTH + border_width*2) * SCALE_X;
    int window_height = (BASE_HEIGHT + border_height*2) * SCALE_Y;
    float aspect_ratio = (float)window_width / (float)window_height;

    display_fullscreen_mode = DISPLAY_WINDOWED_MODE;

    window = SDL_CreateWindow(
        "GSSquared - Apple ][ Emulator", 
        (BASE_WIDTH + border_width*2) * SCALE_X, 
        (BASE_HEIGHT + border_height*2) * SCALE_Y, 
        SDL_WINDOW_RESIZABLE
    );

    if (!window) {
        fprintf(stderr, "Window could not be created! SDL_Error: %s\n", SDL_GetError());
    }

    // Set minimum and maximum window sizes to maintain reasonable dimensions
    SDL_SetWindowMinimumSize(window, window_width / 2, window_height / 2);  // Half size
    SDL_SetWindowMaximumSize(window, window_width * 2, window_height * 2);  // 4x size
    
    // Set the window's aspect ratio to match the Apple II display (560:384)
    SDL_SetWindowAspectRatio(window, aspect_ratio, aspect_ratio);

    for (int i = 0; i < SDL_GetNumRenderDrivers(); i++) {
        const char *name = SDL_GetRenderDriver(i);
        printf("Render driver %d: %s\n", i, name);
    }

    // Create renderer with nearest-neighbor scaling (sharp pixels)
    renderer = SDL_CreateRenderer(window, NULL );
    
    if (!renderer) {
        fprintf(stderr, "Error creating renderer: %s\n", SDL_GetError());
    }

    const char *rname = SDL_GetRendererName(renderer);
    printf("Renderer: %s\n", rname);

    // Set scaling quality to nearest neighbor for sharp pixels
    /* SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); */
    SDL_SetRenderScale(renderer, SCALE_X, SCALE_Y);

    // Clear the texture to black
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    clear();
    present();

    SDL_RaiseWindow(window);
}

video_system_t::~video_system_t() {
    if (renderer) SDL_DestroyRenderer(renderer);
    if (window) SDL_DestroyWindow(window);
    SDL_Quit();
}

void video_system_t::present() {
    SDL_RenderPresent(renderer);
}

void video_system_t::render_frame(SDL_Texture *texture) {
    float w,h;
    SDL_GetTextureSize(texture, &w, &h);
    SDL_FRect dstrect = {
        (float)border_width,
        (float)border_height,
        (float)BASE_WIDTH, 
        (float)BASE_HEIGHT
    };
    SDL_FRect srcrect = {
        (float)0.0,
        (float)0.0,
        (float)w, 
        (float)h
    };
    SDL_RenderTexture(renderer, texture, &srcrect, &dstrect);
}

void video_system_t::clear() {
    SDL_RenderClear(renderer);
}

void video_system_t::window_resize(int new_w, int new_h) {
    // Calculate new scale factors based on window size ratio
    float new_scale_x = (float)new_w / BASE_WIDTH;
    float new_scale_y = (float)new_h / (BASE_HEIGHT + border_height*2);
 
    // TODO: technically this works, but, we should adjust the border_width to center the image.
    // Means borders should be in variable in the display_state_t.
    if (new_scale_x > (new_scale_y / 2.0f)) {
        new_scale_x = new_scale_y / 2.0f;
    }

    border_width = ((new_w / new_scale_x)- BASE_WIDTH) / 2;

    printf("handle_window_resize: new_w: %d, new_h: %d new scale: %f, %f, border w: %d, h: %d\n", new_w, new_h, new_scale_x, new_scale_y, border_width, border_height);

    SDL_SetRenderScale(renderer, new_scale_x, new_scale_y);
}

void video_system_t::toggle_fullscreen() {
    display_fullscreen_mode = (display_fullscreen_mode_t)((display_fullscreen_mode + 1) % NUM_FULLSCREEN_MODES);
    SDL_SetWindowFullscreen(window, display_fullscreen_mode);
}