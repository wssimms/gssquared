#include "gs2.hpp"
#include "cpu.hpp"
#include "bus.hpp"
#include "memory.hpp"
#include "debug.hpp"
#include "parallel.hpp"

void parallel_write_C0x0(cpu_state *cpu, uint16_t addr, uint8_t data) {
    uint8_t slot = (addr - 0xC080) >> 4;
    parallel_data * parallel_d = (parallel_data *)get_slot_state(cpu, (SlotType_t)slot);
    if (DEBUG(DEBUG_PARALLEL)) {
        printf("parallel_write_C0x0 %x\n", data);
    }

    if (parallel_d->output == nullptr) {
        parallel_d->output = fopen("parallel.out", "a");
    }
    fputc(data, parallel_d->output);
}

void parallel_reset(cpu_state *cpu) {
    for (int i = 0; i < 8; i++) {
        parallel_data *parallel_d = (parallel_data *)get_slot_state(cpu, (SlotType_t)i);

        if ((parallel_d != nullptr) && (parallel_d->id == DEVICE_ID_PARALLEL)) {
            if (parallel_d->output != nullptr) {
                fclose(parallel_d->output);
                parallel_d->output = nullptr;
            }
        }
    }
}

void init_slot_parallel(cpu_state *cpu, SlotType_t slot) {
    parallel_data * parallel_d = new parallel_data;
    parallel_d->id = DEVICE_ID_PARALLEL;
    // set in CPU so we can reference later
    
    ResourceFile *rom = new ResourceFile("roms/cards/parallel/parallel.rom", READ_ONLY);
    if (rom == nullptr) {
        fprintf(stderr, "Failed to load parallel.rom\n");
        return;
    }
    rom->load();
    parallel_d->rom = rom;

    set_slot_state(cpu, slot, parallel_d);

    fprintf(stdout, "init_slot_parallel %d\n", slot);

    uint16_t slot_base = 0xC080 + (slot * 0x10);

    register_C0xx_memory_write_handler(slot_base + PARALLEL_DEV, parallel_write_C0x0);
    
    // memory-map the page. Refactor to have a method to get and set memory map.
    uint8_t *rom_data = parallel_d->rom->get_data();

    // load the firmware into the slot memory -- refactor this
    for (int i = 0; i < 256; i++) {
        raw_memory_write(cpu, 0xC000 + (slot * 0x0100) + i, rom_data[i]);
    }
}