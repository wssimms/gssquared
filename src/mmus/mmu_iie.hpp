#pragma once

#include "mmu_ii.hpp"

#include "mbus/MessageBus.hpp"

class MMU_IIe : public MMU_II {
    private:

        MessageBus *mbus;

        virtual void power_on_randomize(uint8_t *ram, int ram_size) override;

    public:
        bool f_intcxrom = false;
        bool f_slotc3rom = false;

        MMU_IIe(int page_table_size, int ram_amount, uint8_t *rom_pointer);
        virtual ~MMU_IIe();
        void set_default_C8xx_map() override;
        void compose_c1cf() override;

        void init_map() override;
        void reset() override;
};

void iie_mmu_handle_C00X_write(void *context, uint16_t address, uint8_t value);
