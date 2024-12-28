#pragma once

#include "../cpu.hpp"

uint64_t init_display();
void free_display();
void txt_memory_write(uint16_t , uint8_t );
void update_display(cpu_state *cpu);
void update_flash_state(cpu_state *cpu);
void init_text_40x24_display();