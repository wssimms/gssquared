#pragma once

#include "gs2.hpp"
#include "util/ResourceFile.hpp"

#define PARALLEL_DEV 0x00

struct parallel_data {
    ResourceFile *rom = nullptr;
    FILE *output = nullptr;
};

void init_slot_parallel(cpu_state *cpu, SlotType_t slot);
void parallel_reset(cpu_state *cpu);