#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    FILE *fp;
    unsigned char buffer[8];
    size_t bytes_read;
    int char_index = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <rom_filename>\n", argv[0]);
        return 1;
    }

    fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("Error opening file");
        return 1;
    }

    // Read 8 bytes at a time (one character)
    while ((bytes_read = fread(buffer, 1, 8, fp)) == 8) {
        // Print character index and file offset
        printf("Character 0x%02X (offset 0x%04X):\n", char_index, char_index * 8);
        
        // Print each line of the character
        for (int line = 0; line < 8; line++) {
            // Process each bit
            for (int bit = 7; bit >= 0; bit--) {
                printf("%c", (buffer[line] & (1 << bit)) ? '*' : '.');
            }
            printf("\n");
        }
        printf("\n");  // Blank line between characters
        
        char_index++;
    }

    fclose(fp);
    return 0;
}
