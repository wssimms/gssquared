#pragma once

#include "gs2.hpp"
#include "cpu.hpp"

#define PD_CMD        0x42
#define PD_DEV        0x43
#define PD_ADDR_LO    0x44
#define PD_ADDR_HI    0x45
#define PD_BLOCK_LO   0x46
#define PD_BLOCK_HI   0x47

#define PD_ERROR_NONE 0x00
#define PD_ERROR_IO   0x27
#define PD_ERROR_NO_DEVICE 0x28
#define PD_ERROR_WRITE_PROTECTED 0x2B

void prodos_block_pv_trap(cpu_state *cpu);
void init_prodos_block(cpu_state *cpu, uint8_t slot);
void mount_prodos_block(uint8_t slot, uint8_t drive, const char *filename);
