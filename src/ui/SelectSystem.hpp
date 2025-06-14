#pragma once

#include <SDL3/SDL.h>
#include "Container.hpp"
#include "AssetAtlas.hpp"
#include "videosystem.hpp"

class SelectSystem {
protected:
    video_system_t *vs;
    Container_t *container;
    bool updated = false;
    TextRenderer *text_renderer;
    int selected_system = 0;
    int window_width, window_height;
    AssetAtlas_t *aa;

public:
    
    /**
     * @brief Constructs the OSD with the given renderer and window.
     * 
     * @param rendererp SDL renderer to use
     * @param windowp SDL window to render to
     * @param window_width Width of the window
     * @param window_height Height of the window
     */
    SelectSystem(video_system_t *vs, AssetAtlas_t *aa);

    ~SelectSystem();

    /**
     * @brief Updates the SelectSystem state
     */
    bool update();

    /**
     * @brief Renders the OSD and all its components.
     */
    void render();

    /**
     * @brief Selects a system from the list.
     */
    int select();

    /**
     * @brief Handles SDL events for the OSD.
     * @param event The SDL event to process
     */
    bool event(const SDL_Event &event);

    void set_raise_window();

    int get_selected_system();
};