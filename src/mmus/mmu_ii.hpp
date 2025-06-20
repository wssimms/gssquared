#pragma once

#include "gs2.hpp"
#include "mmu.hpp"
#include "mmu_ii.hpp"


struct C8XX_handler_t {
    void (*handler)(void *context, SlotType_t slot);
    void *context;
};

class MMU_II : public MMU {
    private:
        int ram_pages;
        uint8_t *main_ram_64 = nullptr;
        uint8_t *main_io_4 = nullptr;
        uint8_t *main_rom_D0 = nullptr;

        read_handler_t C0xx_memory_read_handlers[C0X0_SIZE] = { nullptr };          // Table of memory read handlers
        write_handler_t C0xx_memory_write_handlers[C0X0_SIZE] = { nullptr };        // Table of memory write handlers

        int8_t C8xx_slot;
        C8XX_handler_t C8xx_handlers[8] = {nullptr};

        void power_on_randomize(uint8_t *ram, int ram_size);
        
    public:
        MMU_II(int page_table_size, int ram_amount, uint8_t *rom_pointer);
        ~MMU_II();
        uint8_t read(uint32_t address) override;
        uint8_t floating_bus_read() override;
        void write(uint32_t address, uint8_t value) override;
        
        void set_C8xx_handler(SlotType_t slot, void (*handler)(void *context, SlotType_t slot), void *context);
        void set_C0XX_read_handler(uint16_t address, read_handler_t handler);
        void set_C0XX_write_handler(uint16_t address, write_handler_t handler);
        void call_C8xx_handler(SlotType_t slot);
        uint8_t *get_rom_base();
        void init_map();
        void set_default_C8xx_map();
        void set_slot_rom(SlotType_t slot, uint8_t *rom, const char *name);
        void reset();
        void dump_C0XX_handlers();
};

