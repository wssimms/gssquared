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
    clip_buffer = new uint8_t[sizeof(BMPHeader) + (MAX_CLIP_WIDTH * MAX_CLIP_HEIGHT * 3)];
    header = nullptr;
}

const void *clip_callback(void *userdata, const char *mime_type, size_t *size) {
    // TODO: for now just assume BMP only
    
    ClipboardImage *clip = (ClipboardImage *)userdata;
    size_t calcsize = sizeof(BMPHeader) + (clip->header->infoHeader.width * (clip->header->infoHeader.height * 2) * 3);
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