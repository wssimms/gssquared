#include <iostream>
#include <cstdio>

#include "../gs2.hpp"
#include "../cpu.hpp"
#include "../memory.hpp"
#include "loader.hpp"

char *loader_filename = NULL;
uint16_t loader_address = 0x0000;

void loader_set_file_info(char *filename, uint16_t address) {
    loader_filename = filename;
    loader_address = address;
}

void loader_execute(cpu_state *cpu) {

    if (loader_filename == NULL) {
        fprintf(stderr, "Loader filename not set\n");
        return;
    }

    FILE* file = fopen(loader_filename, "rb");
    if (file == NULL) {
        fprintf(stderr, "Failed to open file %s\n", loader_filename);
        return;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    // Read file into buffer
    uint8_t *memory_chunk = (uint8_t *)malloc(file_size);
    if (memory_chunk == NULL) {
        fprintf(stderr, "Failed to allocate memory for file %s\n", loader_filename);
        fclose(file);
        return;
    }

    fread(memory_chunk, 1, file_size, file);
    fclose(file);

    // Load into RAM at loader_address
    for (long i = 0; i < file_size; i++) {
        raw_memory_write(&CPUs[0], loader_address + i, memory_chunk[i]);
    }

    free(memory_chunk);
}