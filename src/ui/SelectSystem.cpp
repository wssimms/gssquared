#include "SelectSystem.hpp"
#include "Container.hpp"
#include "systemconfig.hpp"
#include "Button.hpp"
#include "Style.hpp"
#include "util/TextRenderer.hpp"
#include "MainAtlas.hpp"

SelectSystem::SelectSystem(SDL_Renderer *rendererp, SDL_Window *windowp, AssetAtlas_t *aa)
    : renderer(rendererp), window(windowp), aa(aa) {

// create container with tiles for each system.
// be cheap right now and do a text button.
Style_t CS;
    CS.background_color = 0x000000FF;
    CS.border_width = 0;
    CS.padding = 50;

    Style_t BS;
    BS.background_color = 0xBFBB98FF;
    BS.border_color = 0xBFBB9840;
    BS.hover_color = 0x008C4AFF;
    BS.padding = 15;
    BS.border_width = 1;
    BS.text_color = 0xFFFFFFFF;

    container = new Container_t(rendererp, 6, CS); 

    container->set_tile_size(1024, 768);
    SDL_GetWindowSize(window, &window_width, &window_height);
    container->set_position((window_width - 1024) / 2, (window_height - 768) / 2);

    text_renderer = new TextRenderer(renderer, "fonts/OpenSans-Regular.ttf", 24);
    selected_system = -1;

    int assets[3] = {Badge_II, Badge_IIPlus, Badge_IIPlus};

    // add a text button for each system.
    for (int i = 0; i < 2; i++) {
        //Button_t *button = new Button_t(BuiltinSystemConfigs[i].name, BS, 0);
        Button_t *button = new Button_t(aa, assets[i], BS, 0);
        button->set_tile_size(200, 200);
        button->position_content(CP_CENTER, CP_CENTER);
        button->set_text_renderer(text_renderer);
        //button->set_position(100 + (i * 200), 100 + (i % 3) * 200);
        button->set_click_callback([this,i](const SDL_Event& event) -> bool {
            selected_system = i;
            return true;
        });
        container->add_tile(button, i);
    }

    container->layout();

    updated = true;
}

bool SelectSystem::event(const SDL_Event &event) {
    container->handle_mouse_event(event);
    return (selected_system != -1);
}

int SelectSystem::get_selected_system() {
    return selected_system;
}

bool SelectSystem::update() {
    return updated;
}

void SelectSystem::render() {
    if (updated) {
        float scale_x, scale_y;
        SDL_GetRenderScale(renderer, &scale_x, &scale_y);
        SDL_SetRenderScale(renderer, 1, 1);
        text_renderer->set_color(255,255,255,255);
        text_renderer->render("Choose your retro experience", (window_width / 2), 50, TEXT_ALIGN_CENTER);

        container->render();
        SDL_SetRenderScale(renderer, scale_x, scale_y);
        //updated = false;
    }
}
