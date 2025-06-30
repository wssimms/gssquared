#pragma once

#include "frame.hpp"
#include "devices/displaypp/RGBA.hpp"

//typedef uint32_t RGBA_t;

using Frame560 = Frame<uint8_t, 192, 580>;
using Frame560RGBA = Frame<RGBA_t, 192, 580>;
