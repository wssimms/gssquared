#include "Render.hpp"

class Monochrome560 : public Render {

public:
    Monochrome560() {};
    ~Monochrome560() {};

    void render(Frame560 *frame_byte, Frame560RGBA *frame_rgba, RGBA_t color) {
        for (int l = 0; l < 192; l++) {
            frame_rgba->set_line(l);
            frame_byte->set_line(l);
            uint16_t fw = frame_byte->width();
            for (int i = 0; i < fw; i++) {
                frame_rgba->push(frame_byte->pull() ? color : (RGBA_t){.rgba = 0x00000000});
            }
        }
    };
};
