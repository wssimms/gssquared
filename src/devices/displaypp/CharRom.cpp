#include <cstdint>
#include <cstdio>
#include <new>

#include "CharRom.hpp"


CharRom::CharRom(const char *filename) {
    data = new(std::align_val_t(64)) uint8_t[8192]; // max char rom size

    FILE *f = fopen(filename, "rb");
    if (!f) {
        printf("Failed to open char rom file: %s\n", filename);
        valid = false;
        delete[] data;
        return;
    }

    size = fread(data, 1, 8192, f);
    fclose(f);
    if (size != 2048 && size != 4096 && size != 8192) {
        printf("Invalid char rom file: %s\n", filename);
        valid = false;
        delete[] data;
        return;
    }

    // if size == 2048, it's an Apple II Plus Char ROM, and we need to reverse the bits.
    if (size == 2048) {
        for (int i = 0; i < 2048; i++) {
            data[i] = reverse_bits(data[i]);
        }
    } else { // iie and on roms, we need to invert the bits
        for (int i = 0; i < size; i++) {
            data[i] = invert_bits(data[i]);
        }
    }
    valid = true;
}

CharRom::~CharRom() {
    delete[] data;
}

uint8_t CharRom::reverse_bits(uint8_t b) {
    // in apple ii+, the high bit set on a character line means it's a flash character.
    // we handle that differently by decoding the char & 0xC0 == 0x40. 
    uint8_t r = 0;
    for (int i = 0; i < 7; i++) {
        bool bit = b & (1 << (6-i)); 
        r |= (bit << i);
    }
    return r;
}

void CharRom::print_matrix(uint16_t tchar) {
    for (int line = 0; line < 8; line++) {
        uint8_t cdata = get_char_scanline(tchar, line);
        // Process each bit
        for (int bit = 0; bit < 7; bit++) {
            printf("%c", (cdata & 1) ? '*' : '.');
            cdata >>= 1;
        }
    printf("\n");
}
}
