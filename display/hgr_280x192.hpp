#pragma once

#include <stdint.h>
#include "cpu.hpp"

extern uint16_t HGR_PAGE_START;
extern uint16_t HGR_PAGE_END;
extern uint16_t *HGR_PAGE_TABLE;

extern uint16_t HGR_PAGE_1_TABLE[24];
extern uint16_t HGR_PAGE_2_TABLE[24];

void render_hgr(cpu_state *cpu, int x, int y, void *pixels, int pitch);
void hgr_memory_write(uint16_t address, uint8_t value);
