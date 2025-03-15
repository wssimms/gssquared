#include "gs2.hpp"
#include "systemconfig.hpp"
#include "slots.hpp"


SlotManager_t::SlotManager_t() {
    for (uint8_t i = 0; i < MAX_SLOTS; i++) {
        Slots[i].slot_number = i;
        Slots[i].card = NULL;
    }
}

SlotManager_t::~SlotManager_t() {
    for (uint8_t i = 0; i < MAX_SLOTS; i++) {
        if (Slots[i].card != NULL) {
            unregister_slot(i);
        }
    }
}

/**
 * Register a device in a slot.
 * @param device The device to register.
 * @param slot_number The slot number to register the device in.
 */
void SlotManager_t::register_slot(Device_t *device, uint8_t slot_number) {
    if (slot_number >= MAX_SLOTS) {
        return;
    }
    Slots[slot_number].card = device;
}

/**
 * Unregister a device from a slot.
 * @param slot_number The slot number to unregister the device from.
 */
void SlotManager_t::unregister_slot(uint8_t slot_number) {
    if (slot_number >= MAX_SLOTS) {
        return;
    }
    Slots[slot_number].card = NULL;
}

/**
 * Get the device in a slot.
 * @param slot_number The slot number to get the device from.
 * @return The device in the slot.
 */
Device_t *SlotManager_t::get_device(uint8_t slot_number) {  
    if (slot_number >= MAX_SLOTS) {
        return NULL;
    }
    if (Slots[slot_number].card == NULL) {
        return &NoDevice;
    }
    return Slots[slot_number].card;
}
