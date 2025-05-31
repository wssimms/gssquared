#include <SDL3/SDL.h>

#include "debugwindow.hpp"
#include "gs2.hpp"
#include "cpu.hpp"
#include "util/TextRenderer.hpp"
#include "computer.hpp"

debug_window_t::debug_window_t(computer_t *computer) {
    this->computer = computer;
    this->cpu = computer->cpu;

    // create a new window
    window = SDL_CreateWindow("Debug Window", window_width, window_height, SDL_WINDOW_RESIZABLE|SDL_WINDOW_HIDDEN);
    // create a new renderer
    renderer = SDL_CreateRenderer(window, nullptr);

    text_renderer = new TextRenderer(renderer, "/fonts/OxygenMono-Regular.ttf", 15.0f);
    font_line_height = text_renderer->font_line_height;

    window_id = SDL_GetWindowID(window);
    control_area_height = 8 * font_line_height;
    lines_in_view_area = (window_height - control_area_height) / font_line_height;
}

void debug_window_t::separator_line(int y) {
    SDL_RenderLine(renderer, 0, (y*font_line_height)-3, window_width, (y*font_line_height)-3);
}

void debug_window_t::draw_text(int y, const char *textToShow) {
    text_renderer->render(textToShow, window_margin, y*font_line_height);
}

void debug_window_t::render() {
    char buffer[256];

    if (!window_open) {
        return;
    }

    SDL_SetRenderDrawColor(renderer, 0,0,0, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);

    int numlines = (window_height - control_area_height) / font_line_height;
    size_t head = cpu->trace_buffer->head;
    size_t size = cpu->trace_buffer->size;
    
    // Calculate the starting index to show the last numlines entries
    // We need to handle wrapping around the circular buffer
    size_t start_idx = (head + size - numlines - view_position) % size;
    
    for (int i = 0; i < numlines; i++) {
        size_t idx = (start_idx + i) % size;
        char *line = cpu->trace_buffer->decode_trace_entry(&cpu->trace_buffer->entries[idx]);
        draw_text(8 + i, line);
    }

    separator_line(3);
    snprintf(buffer, sizeof(buffer), "T)race: %s  SPACE: Step  RETURN: Run  Up/Dn/PgUp/PgDn/Home/End: Scroll", cpu->trace ? "ON " : "OFF");
    draw_text(3, buffer);
   

    separator_line(4);
    draw_text(4, "PC     A  X  Y  SP     N V - D B I Z C");
    snprintf(buffer, sizeof(buffer), "%04X   %02X %02X %02X %04X   %d %d - %d %d %d %d %d", cpu->pc, cpu->a, cpu->x, cpu->y, cpu->sp, 
        cpu->N, cpu->V, cpu->B, cpu->D, cpu->I, cpu->Z, cpu->C);
    draw_text(5, buffer);

    snprintf(buffer, sizeof(buffer), "Cycles: %18llu    MHz: %10.5f", cpu->cycles, cpu->e_mhz);
    draw_text(6, buffer);

    separator_line(7);
    draw_text(7, "   Cycle             A  X  Y  SP   P     PC                                   Eff  M");
    separator_line(8);

    // draw location bar (not exactly a scrollbar)
    // Calculate number of lines entries distance of view_position is from tail
    size_t buffer_size = cpu->trace_buffer->count;
    // view_position is position of bottom line of view area. 0 = head. -size = tail basically.
    float bar_height = window_height - control_area_height;
    float bar_fraction = 1 - (float)view_position / buffer_size; // it's the inverse of the fraction since we want it to be at the bottom
    float bar_divider = (bar_fraction * bar_height);
    SDL_FRect bar_rect = {window_width - 10.0f, (float)control_area_height, 10.0f, bar_height};
    SDL_RenderRect(renderer, &bar_rect);
    bar_rect.y = control_area_height + bar_divider;
    bar_rect.h = bar_height - bar_divider;
    SDL_SetRenderDrawColor(renderer, 0, 255, 255, 255);
    SDL_RenderFillRect(renderer, &bar_rect);

    SDL_RenderPresent(renderer);
}

void debug_window_t::resize(int width, int height) {
    window_width = width;
    window_height = height;
    lines_in_view_area = (window_height - control_area_height) / font_line_height;
}

bool debug_window_t::handle_event(SDL_Event &event) {
    if (event.window.windowID == window_id) {
        switch (event.type) {
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                set_closed();
                break;
            case SDL_EVENT_WINDOW_RESIZED:
                resize(event.window.data1, event.window.data2);
                break;
            case SDL_EVENT_KEY_DOWN:
                if (event.key.key == SDLK_SPACE) {
                    cpu->execution_mode = EXEC_STEP_INTO;
                    cpu->instructions_left = 1;
                }
                if (event.key.key == SDLK_RETURN) {
                    cpu->execution_mode = EXEC_NORMAL;
                    cpu->instructions_left = 0;
                    view_position = 0;
                }
                if (event.key.key == SDLK_KP_ENTER && event.key.mod & SDL_KMOD_CTRL) {
                    if (window_open) {
                        set_closed();
                    } else {
                        set_open();
                    }
                }
                if (event.key.key == SDLK_UP) {
                    view_position++;
                }
                if (event.key.key == SDLK_DOWN) {
                    view_position--;
                    if (view_position < 0) {
                        view_position = 0;
                    }
                }
                if (event.key.key == SDLK_PAGEUP) {
                    view_position += lines_in_view_area;
                }
                if (event.key.key == SDLK_PAGEDOWN) {
                    view_position -= lines_in_view_area;
                    if (view_position < 0) {
                        view_position = 0;
                    }
                }
                if (event.key.key == SDLK_HOME) {
                    view_position = cpu->trace_buffer->count - lines_in_view_area;
                }
                if (event.key.key == SDLK_END) {
                    view_position = 0;
                }
                if (event.key.key == SDLK_T) {
                    cpu->trace = !cpu->trace;
                }

                break;
        }
        return true;
    } else {
        if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_KP_ENTER && event.key.mod & SDL_KMOD_CTRL) {
            if (window_open) {
                set_closed();
            } else {
                set_open();
            }
        }
    }
    return false;
}

bool debug_window_t::is_open() {
    return window_open;
}

void debug_window_t::set_open() {
    window_open = true;
    computer->video_system->show(window);
    computer->video_system->raise(window);
    //SDL_ShowWindow(window);
    //SDL_RaiseWindow(window);
}

void debug_window_t::set_closed() {
    window_open = false;

    computer->video_system->hide(window);
    computer->video_system->raise(computer->video_system->window); // TODO: awkward.

    // SDL_HideWindow(window);
    // SDL_RaiseWindow(video_system->window);
}