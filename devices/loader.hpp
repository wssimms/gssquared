#pragma once

#include <stdint.h>
#include "../cpu.hpp"

void loader_execute(cpu_state *cpu);
void loader_set_file_info(char *filename, uint16_t address);
