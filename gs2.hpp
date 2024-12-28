#pragma once

#define USE_SDL2 1

#include <unistd.h>
#include <stdint.h>

/* Address types */
typedef uint8_t zpaddr_t;
typedef uint16_t absaddr_t;

/* Data bus types */
typedef uint8_t byte_t;
typedef uint16_t word_t;
typedef uint8_t opcode_t;
