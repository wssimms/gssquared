#include "gs2.hpp"
#include "systemconfig.hpp"
#include "slots.hpp"


SlotManager_t::SlotManager_t() {
    for (int i = SLOT_0; i < NUM_SLOTS; i++) {
        Slots[static_cast<SlotType_t>(i)].slot_number = static_cast<SlotType_t>(i);
        Slots[static_cast<SlotType_t>(i)].card = NULL;
    }
}

SlotManager_t::~SlotManager_t() {
    for (int i = SLOT_0; i < NUM_SLOTS; i++) {
        if (Slots[static_cast<SlotType_t>(i)].card != NULL) {
            unregister_slot(static_cast<SlotType_t>(i));
        }
    }
}

/**
 * Register a device in a slot.
 * @param device The device to register.
 * @param slot_number The slot number to register the device in.
 */
void SlotManager_t::register_slot(Device_t *device, SlotType_t slot_number) {
    if (slot_number >= NUM_SLOTS) {
        return;
    }
    Slots[slot_number].card = device;
}

/**
 * Unregister a device from a slot.
 * @param slot_number The slot number to unregister the device from.
 */
void SlotManager_t::unregister_slot(SlotType_t slot_number) {
    if (slot_number >= NUM_SLOTS) {
        return;
    }
    Slots[slot_number].card = NULL;
}

/**
 * Get the device in a slot.
 * @param slot_number The slot number to get the device from.
 * @return The device in the slot.
 */
Device_t *SlotManager_t::get_device(SlotType_t slot_number) {  
    if (slot_number >= NUM_SLOTS) {
        return NULL;
    }
    if (Slots[slot_number].card == NULL) {
        return &NoDevice;
    }
    return Slots[slot_number].card;
}
