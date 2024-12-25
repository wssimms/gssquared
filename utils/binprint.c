#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    FILE *fp;
    unsigned char buffer[16];
    size_t bytes_read;
    long offset = 0;

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    fp = fopen(argv[1], "rb");
    if (!fp) {
        perror("Error opening file");
        return 1;
    }

    while ((bytes_read = fread(buffer, 1, 16, fp)) > 0) {
        printf("%06lX: ", offset);
        
        // Print hex values
        for (size_t i = 0; i < 16; i++) {
            if (i < bytes_read) {
                printf("%02X", buffer[i]);
            } else {
                printf("  ");
            }
            
            if ((i + 1) % 16 != 0) {
                printf(" ");
                if ((i + 1) % 4 == 0) {
                    printf(" ");
                }
            }
        }

        printf("  ");

        // Print ASCII representation
        for (size_t i = 0; i < bytes_read; i++) {
            if (buffer[i] >= 32 && buffer[i] <= 126) {
                printf("%c ", buffer[i]);
            } else {
                printf(". ");
            }
        }
        
        printf("\n");
        offset += 16;
    }

    fclose(fp);
    return 0;
}
