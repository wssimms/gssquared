#include <stdio.h>
#include "gs2.hpp"
#include "cpu.hpp"
#include "memory.hpp"
#include "debug.hpp"
#include "devices/prodos_block/prodos_block.hpp"
#include "pd_block_firmware.hpp"


typedef struct media_t {
    FILE *file;
    uint16_t block_size;
    uint16_t block_count;
} media_t;

media_t media[7][2];


void pv_return(cpu_state *cpu) {
    // pop two bytes off the stack.
    // TODO: These need to do proper bounds checking
    uint8_t pcl = read_memory(cpu, 0x0100 + cpu->sp+1);
    uint8_t pch = read_memory(cpu, 0x0100 + cpu->sp+2);
    cpu->sp += 2;
    cpu->pc = (pcl | (pch << 8)) + 1;
    printf("ParaVirtual Return PC: %04X SP: A: %02X C: %02X\n", cpu->pc, cpu->a_lo, cpu->C);
}

uint8_t status(cpu_state *cpu, uint8_t slot, uint8_t drive) {
    if (media[slot][drive].file == nullptr) {
        return 0x01; // device not ready
    }
    return 0x00; // device ready
}

void read_block(cpu_state *cpu, uint8_t slot, uint8_t drive, uint16_t block, uint16_t addr) {
    // TODO: read the block into the address.
    static uint8_t block_buffer[512];
    FILE *fp = media[slot][drive].file;
    
    fseek(fp, block * media[slot][drive].block_size, SEEK_SET);
    fread(block_buffer, 1, media[slot][drive].block_size, fp);
    for (int i = 0; i < media[slot][drive].block_size; i++) {
        // TODO: for dma we want to simulate the memory map but do not want to burn cycles.
        write_memory(cpu, addr + i, block_buffer[i]); 
    }
    //debug_dump_memory(cpu, addr, addr + media[slot][drive].block_size);
}

void write_block(cpu_state *cpu, uint8_t slot, uint8_t drive, uint16_t block, uint16_t addr) {
    // TODO: read the block into the address.
    static uint8_t block_buffer[512];
    FILE *fp = media[slot][drive].file;
    
    for (int i = 0; i < media[slot][drive].block_size; i++) {
        // TODO: for dma we want to simulate the memory map but do not want to burn cycles.
        block_buffer[i] = read_memory(cpu, addr + i); 
    }
    fseek(fp, block * media[slot][drive].block_size, SEEK_SET);
    fwrite(block_buffer, 1, media[slot][drive].block_size, fp);

    //debug_dump_memory(cpu, addr, addr + media[slot][drive].block_size);
}

void prodos_block_pv_trap(cpu_state *cpu) {
    uint16_t block = raw_memory_read(cpu, PD_BLOCK_LO) | (raw_memory_read(cpu, PD_BLOCK_HI) << 8);
    uint16_t addr = raw_memory_read(cpu, PD_ADDR_LO) | (raw_memory_read(cpu, PD_ADDR_HI) << 8);
    uint8_t unit = raw_memory_read(cpu, PD_DEV);
    uint8_t slot = (unit >> 4) & 0b0111;
    uint8_t drive = (unit >> 7) & 0b1;
    uint8_t cmd = raw_memory_read(cpu, PD_CMD);
    printf("ParaVirtual Trap PC: %04X, Unit: %02X, Block: %04X, Addr: %04X, CMD: %02X\n", cpu->pc, unit, block, addr, cmd);

    if (cmd == 0x00) {
        uint8_t st = status(cpu, slot, drive);
        if (st) {
            cpu->C = 1; // set error flag.
            cpu->a_lo = PD_ERROR_NO_DEVICE;
        } else {
            cpu->C = 0; // clear error flag.
            cpu->a_lo = 0x00;
            cpu->x_lo = 0x40;
            cpu->y_lo = 0x06;
        }
    } else if (cmd == 0x01) {
        uint8_t st = status(cpu, slot, drive);
        if (st) {
            cpu->C = 1; // set error flag.
            cpu->a_lo = PD_ERROR_NO_DEVICE;
        } else {
            read_block(cpu, slot, drive, block, addr);
            cpu->C = 0; // clear carry
            cpu->a_lo = 0x00;
        }
    } else if (cmd == 0x02) {
        uint8_t st = status(cpu, slot, drive);
        if (st) {
            cpu->C = 1; // set error flag.
            cpu->a_lo = PD_ERROR_NO_DEVICE;
        } else {
            write_block(cpu, slot, drive, block, addr);
            cpu->C = 0; // clear carry
            cpu->a_lo = 0x00;
        }
    } else if (cmd == 0x03) { // 
        cpu->C = 1; // set error flag. // format not implemented, TODO
        cpu->a_lo = PD_ERROR_NO_DEVICE;
    }
    pv_return(cpu);
}

void mount_prodos_block(uint8_t slot, uint8_t drive, const char *filename) {
    printf("Mounting ProDOS block device %s slot %d drive %d\n", filename, slot, drive);

    FILE *fp = fopen(filename, "r+b");
    if (fp == nullptr) {
        fprintf(stderr, "Could not open ProDOS block device file: %s\n", filename);
        return;
    }
    media[slot][drive].file = fp;
    media[slot][drive].block_size = 512;
    media[slot][drive].block_count = 1600;

}

void init_prodos_block(cpu_state *cpu, uint8_t slot)
{

    printf("Prodos Block Firmware:\n");
    for (int i = 0; i < 256; i++)
    {
        printf("%02X ", pd_block_firmware[i]);
        if (i % 16 == 0x0f)
        {
            printf("\n");
        }

        // load the firmware into the slot memory
        for (int i = 0; i < 256; i++) {
            raw_memory_write(cpu, 0xC000 + (slot * 0x0100) + i, pd_block_firmware[i]);
        }

        /* for (int slot = 0; slot <= 7; slot++) {
            for (int drive = 0; drive <= 1; drive++) {
                media[slot][drive].file = NULL;
                media[slot][drive].block_size = 0;
                media[slot][drive].block_count = 0;
            }
        } */
    }

}
