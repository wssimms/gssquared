#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include "cpu.hpp"
#include "bus.hpp"

/*

https://retrocomputing.stackexchange.com/questions/5512/is-the-apple-ii-no-slot-clock-compatible-with-the-thunderclock

*/

#define THUNDERCLOCK_CMD_REG_BASE 0xC080

#define TCP_IN   0b00000001
#define TCP_CLK  0b00000010
#define TCP_STB  0b00000100
#define TCP_CMD  0b00111000
#define TCP_INTR 0b01000000
#define TCP_INTR_ASSERT 0b00100000
#define TCP_OUT  0b10000000

#define TCP_CMD_READ_TIME 0b00011000
#define TCP_CMD_SET_TIME *unknown*

#define HZ64 64
#define HZ256 256
#define HZ1024 1024

uint8_t thunderclock_command_register = 0;
uint64_t thunderclock_time_register = 0;
uint8_t thunderclock_interrupt_asserted = 0;
uint8_t thunderblock_interrupt_rate = HZ64;


// Returns 40 bits of time data in Thunderclock Plus format
// the LSB of our 40-bit register is the LSB of the seconds-units field.
uint64_t get_thunderclock_time() {
    time_t now = time(nullptr);
    struct tm *tm = localtime(&now);
    
    // First collect nibbles in order
    uint8_t nibbles[10] = {
        (uint8_t)(tm->tm_mon + 1),    // month (1-12 binary)
        (uint8_t)tm->tm_wday,         // day of week (0-6 BCD)
        (uint8_t)(tm->tm_mday / 10),  // date tens
        (uint8_t)(tm->tm_mday % 10),  // date units
        (uint8_t)(tm->tm_hour / 10),  // hour tens
        (uint8_t)(tm->tm_hour % 10),  // hour units
        (uint8_t)(tm->tm_min / 10),   // minute tens
        (uint8_t)(tm->tm_min % 10),   // minute units
        (uint8_t)(tm->tm_sec / 10),   // second tens
        (uint8_t)(tm->tm_sec % 10)    // second units
    };

    // Pack nibbles into result, LSB first
    uint64_t result = 0;
    for (int i = 0; i < 10; i++) {
        result = (result << 4) | (nibbles[i] & 0x0F);
    }
    
    return result;
}

/**
 * The first bit we fetch is the LSB of the seconds-units field. 
 */
uint8_t thunderclock_read_register(cpu_state *cpu, uint16_t address) {
    fprintf(stderr, "Thunderclock Plus read register %04X => %02X\n", address, thunderclock_command_register);
    
    uint8_t bit = (thunderclock_time_register & 0x01) << 7;
    uint8_t reg = thunderclock_command_register;
    reg = (reg & (~TCP_OUT)) | bit;
    return reg;
}

void thunderclock_write_register(cpu_state *cpu, uint16_t address, uint8_t value) {
    fprintf(stderr, "Thunderclock Plus write register %X value %X\n", address, value);
    // check for strobe HI to LO transition. Then perform commmand.
    if ((thunderclock_command_register & TCP_STB) && ((value & TCP_STB) == 0)) {
        // read the command register.
        if ((value & TCP_CMD) == TCP_CMD_READ_TIME) {
            thunderclock_time_register = get_thunderclock_time();
            fprintf(stderr, "Thunderclock Plus read time: %llX\n", thunderclock_time_register);
        }
    }
    if ((thunderclock_command_register & TCP_CLK) && ((value & TCP_CLK) == 0)) {
        // shift the time register right.
        fprintf(stderr, "Thunderclock Plus CLK tick - shift time right\n");
        thunderclock_time_register >>= 1;
    }

    // remember the value.
    thunderclock_command_register = value;

}

void init_thunderclock(int slot) {
    uint16_t thunderclock_cmd_reg = THUNDERCLOCK_CMD_REG_BASE + (slot << 4);
    fprintf(stderr, "Thunderclock Plus init at SLOT %d address %X\n", slot, thunderclock_cmd_reg);
    register_C0xx_memory_read_handler(thunderclock_cmd_reg, thunderclock_read_register);
    register_C0xx_memory_write_handler(thunderclock_cmd_reg, thunderclock_write_register);
}
