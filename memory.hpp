#pragma once

#include <stdint.h>
#include "cpu.hpp"


 uint8_t raw_memory_read(cpu_state *cpu, uint16_t address);
 void raw_memory_write(cpu_state *cpu, uint16_t address, uint8_t value);
 void raw_memory_write_word(cpu_state *cpu, uint16_t address, uint16_t value);

uint8_t read_memory(cpu_state *cpu, uint16_t address);
void write_memory(cpu_state *cpu, uint16_t address, uint8_t value);
void memory_bus_read(uint16_t address, uint8_t value);
void memory_bus_write(uint16_t address, uint8_t value);
uint8_t read_byte(cpu_state *cpu, uint16_t address);
uint16_t read_word(cpu_state *cpu, uint16_t address);
uint16_t read_word_from_pc(cpu_state *cpu);
void write_byte(cpu_state *cpu, uint16_t address, uint8_t value);
void write_word(cpu_state *cpu, uint16_t address, uint16_t value);
void store_byte(cpu_state *cpu, uint16_t address, uint8_t value);
void store_word(cpu_state *cpu, uint16_t address, uint16_t value);
 uint8_t read_byte_from_pc(cpu_state *cpu);
