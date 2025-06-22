#pragma once

#include "devices/displaypp/frame/frame.hpp"
#include "devices/displaypp/frame/Frames.hpp"

class Render {

    public:
        Render() {};
        ~Render() {};

        void render(Frame560 *frame_byte, Frame560RGBA *frame_rgba);

    private:
        
};