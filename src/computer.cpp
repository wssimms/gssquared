#include "gs2.hpp"
//#include "reset.hpp"
#include "cpu.hpp"
#include "devices/diskii/diskii.hpp"
#include "devices/languagecard/languagecard.hpp"
#include "devices/parallel/parallel.hpp"
#include "memory.hpp"
#include "devices/mockingboard/mb.hpp"

#include "computer.hpp"

computer_t::computer_t() {
    cpu = new cpu_state();
    cpu->init();
    video_system = new video_system_t();
}

computer_t::~computer_t() {
    delete cpu;
    delete video_system;
}

void computer_t::reset(bool cold_start) {
    // TODO: implement register_reset_handler so a device can register a reset handler.
    // and when system_reset is called, it will call all the registered reset handlers.

    // TODO: this is a higher level concept than just resetting the CPU.
    if (cold_start) {
        // force a cold start reset
        raw_memory_write(cpu, 0x3f2, 0x00);
        raw_memory_write(cpu, 0x3f3, 0x00);
        raw_memory_write(cpu, 0x3f4, 0x00);
    }

    cpu->reset();
    diskii_reset(cpu);
    cpu->init_default_memory_map();
    reset_languagecard(cpu); // reset language card
    parallel_reset(cpu);
#if MOCKINGBOARD_ENABLED
    mb_reset(cpu);
#endif
}
