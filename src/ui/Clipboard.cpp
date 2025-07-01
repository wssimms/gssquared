#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <SDL3/SDL_clipboard.h>

#include "computer.hpp"
#include "display/display.hpp"
#include "Clipboard.hpp"

#define MAX_CLIP_WIDTH (640)
#define MAX_CLIP_HEIGHT (216*2)

ClipboardImage::ClipboardImage( ) {
    uint32_t clip_data_size = MAX_CLIP_WIDTH * MAX_CLIP_HEIGHT * 3;
    clip_buffer = new uint8_t[sizeof(BMPHeader) + clip_data_size];
    header = nullptr;
}

const void *clip_callback(void *userdata, const char *mime_type, size_t *size) {
    // TODO: for now just assume BMP only
    
    ClipboardImage *clip = (ClipboardImage *)userdata;
    uint32_t bytes_per_line = clip->header->infoHeader.width * 3;
    // Round bytes_per_line up to the next multiple of 4
    if (bytes_per_line % 4 != 0) {
        bytes_per_line = ((bytes_per_line + 3) / 4) * 4;
    }

    size_t calcsize = sizeof(BMPHeader) + (bytes_per_line * (clip->header->infoHeader.height) );
    //printf("clip_callback: %s (%d)\n", mime_type, calcsize);
    *size = calcsize;
    return clip->clip_buffer;
}

void ClipboardImage::Clip(Display * display) {
    uint32_t width, height;
    const char *mime_types[] = { "image/bmp" };

    display->get_buffer(clip_buffer + sizeof(BMPHeader), &width, &height);

    if (header != nullptr) {
        delete header;
        header = nullptr;
    }
    header = new BMPHeader(width, height);
    memcpy(clip_buffer, header, sizeof(BMPHeader));
    bool res = SDL_SetClipboardData(clip_callback, nullptr, (void *)this, mime_types, 1 );
    if (!res) {
        printf("Failed to set clipboard data: %s\n", SDL_GetError());
    }
}