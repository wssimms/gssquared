#pragma once

#include <vector>

#include "mmus/mmu_ii.hpp"
#include "Module_ID.hpp"
#include "SlotData.hpp"
#include "util/EventDispatcher.hpp"
#include "util/EventQueue.hpp"

struct cpu_state;
struct debug_window_t; // don't bring in debugwindow.hpp, it would create a depedence on SDL.
struct video_system_t; // same.
class Mounts;
class EventTimer;

typedef void (*reset_handler_t)(void *context);

struct reset_handler_rec {
    reset_handler_t handler;
    void *context;
};

struct computer_t {
    cpu_state *cpu = nullptr;
    MMU_II *mmu = nullptr;

    EventDispatcher *sys_event = nullptr;
    EventDispatcher *dispatch = nullptr;

    video_system_t *video_system = nullptr;
    debug_window_t *debug_window = nullptr;

    EventTimer *event_timer = nullptr;

    EventQueue *event_queue = nullptr;
    
    Mounts *mounts = nullptr;

    std::vector<reset_handler_rec> reset_handlers;

    void *module_store[MODULE_NUM_MODULES];
    SlotData *slot_store[NUM_SLOTS];

    computer_t();
    ~computer_t();
    void set_mmu(MMU_II *mmu) { this->mmu = mmu; }
    void reset(bool cold_start);

    void register_reset_handler(reset_handler_rec rec);

    void *get_module_state( module_id_t module_id);
    void set_module_state( module_id_t module_id, void *state);

    SlotData *get_slot_state(SlotType_t slot);
    SlotData *get_slot_state_by_id(device_id id);
    void set_slot_state( SlotType_t slot, SlotData *state);

    void set_slot_irq( uint8_t slot, bool irq);

    void send_clock_mode_message();

};