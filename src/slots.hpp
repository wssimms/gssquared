#pragma once

#include "gs2.hpp"
#include "systemconfig.hpp"

#define MAX_SLOTS 8

struct Slot_t {
    uint8_t slot_number;
    Device_t *card;
};

class SlotManager_t {
    Slot_t Slots[MAX_SLOTS];
    
    public:
        SlotManager_t();
        ~SlotManager_t();
        void register_slot(Device_t *device, uint8_t slot_number);
        void unregister_slot(uint8_t slot_number);
        Device_t *get_device(uint8_t slot_number);
};
