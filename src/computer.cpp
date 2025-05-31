#include "gs2.hpp"
//#include "reset.hpp"
#include "cpu.hpp"
#include "devices/diskii/diskii.hpp"
#include "devices/languagecard/languagecard.hpp"
#include "devices/parallel/parallel.hpp"
#include "devices/mockingboard/mb.hpp"

#include "computer.hpp"
#include "debugger/debugwindow.hpp"

computer_t::computer_t() {
    cpu = new cpu_state();
    cpu->init();
    video_system = new video_system_t();
    debug_window = new debug_window_t(this);
}

computer_t::~computer_t() {
    delete cpu;
    delete video_system;
}

void computer_t::register_reset_handler(reset_handler_rec rec) {
    reset_handlers.push_back(rec);
}

void computer_t::reset(bool cold_start) {

    if (cold_start) {
        // force a cold start reset
        cpu->mmu->write(0x3f2, 0x00);
        cpu->mmu->write(0x3f3, 0x00);
        cpu->mmu->write(0x3f4, 0x00);
    }

    cpu->reset();
    mmu->init_map();

    for (reset_handler_rec rec : reset_handlers) {
        rec.handler(rec.context);
    }
}

#if 0
void computer_t::reset(bool cold_start) {
    // TODO: implement register_reset_handler so a device can register a reset handler.
    // and when system_reset is called, it will call all the registered reset handlers.

    // TODO: this is a higher level concept than just resetting the CPU.
    if (cold_start) {
        // force a cold start reset
        cpu->mmu->write(0x3f2, 0x00);
        cpu->mmu->write(0x3f3, 0x00);
        cpu->mmu->write(0x3f4, 0x00);
    }

    cpu->reset();
    diskii_reset(cpu);
    //cpu->init_default_memory_map();
    reset_languagecard(cpu); // reset language card
    parallel_reset(cpu);
#if MOCKINGBOARD_ENABLED
    mb_reset(cpu);
#endif
}
#endif

/** State storage for non-slot devices. */
void *computer_t::get_module_state(module_id_t module_id) {
    void *state = cpu->module_store[module_id];
    if (state == nullptr) {
        fprintf(stderr, "Module %d not initialized\n", module_id);
    }
    return state;
}

void computer_t::set_module_state(module_id_t module_id, void *state) {
    cpu->module_store[module_id] = state;
}

/** State storage for slot devices. */
SlotData *computer_t::get_slot_state(SlotType_t slot) {
    SlotData *state = cpu->slot_store[slot];
    /* if (state == nullptr) {
        fprintf(stderr, "Slot Data for slot %d not initialized\n", slot);
    } */
    return state;
}

SlotData *computer_t::get_slot_state_by_id(device_id id) {
    for (int i = 0; i < 8; i++) {
        if (cpu->slot_store[i] && cpu->slot_store[i]->id == id) {
            return cpu->slot_store[i];
        }
    }
    return nullptr;
}

void computer_t::set_slot_state( SlotType_t slot, /* void */ SlotData *state) {
    state->_slot = slot;
    cpu->slot_store[slot] = state;
}

void computer_t::set_slot_irq(uint8_t slot, bool irq) {
    if (irq) {
        cpu->irq_asserted |= (1 << slot);
    } else {
        cpu->irq_asserted &= ~(1 << slot);
    }
}