#pragma once

#include "cpu.hpp"

struct computer_t {
    cpu_state *cpu = nullptr;
    video_system_t *video_system;

    computer_t();
    ~computer_t();
    void reset(bool cold_start);
};