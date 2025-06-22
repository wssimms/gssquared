#pragma once

#include <cstdint>
#include <cstdio>

class CharRom {
    public:
        CharRom(uint8_t *data, int size) {
            this->data = data;
            this->size = size;
            valid = true;
        }

        CharRom(const char *filename);

        ~CharRom();

        inline uint8_t get_char_scanline(uint16_t tchar, uint8_t y) {
            uint16_t index = tchar * 8 + y;
            uint8_t byte = data[index];
            return byte;
        }

        uint8_t *get_data() {
            return data;
        }

        bool is_valid() {
            return valid;
        }

        void print_matrix(uint16_t tchar);

    private:
        uint8_t *data = nullptr;
        int size = 0;
        bool valid = false;

        inline uint8_t invert_bits(uint8_t b) {
            return ~b;
        }

        uint8_t reverse_bits(uint8_t b);
};