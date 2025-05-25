#include "computer.hpp"

computer_t::computer_t() {
    cpu = new cpu_state();
    cpu->init();
}

computer_t::~computer_t() {
    delete cpu;
}
