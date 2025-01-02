#pragma once

#include "../cpu.hpp"

extern uint16_t TEXT_PAGE_START;
extern uint16_t TEXT_PAGE_END;
extern uint16_t *TEXT_PAGE_TABLE;
extern uint16_t TEXT_PAGE_1_TABLE[24];
extern uint16_t TEXT_PAGE_2_TABLE[24];

void txt_memory_write(uint16_t , uint8_t );
void update_display(cpu_state *cpu);
void update_flash_state(cpu_state *cpu);
//void load_character_rom();
void render_text(cpu_state *cpu, int x, int y, void *pixels, int pitch);
//void set_text_page1();
//void set_text_page2();
