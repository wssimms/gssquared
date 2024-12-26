#include "cpu.hpp"
#include "memory.hpp"

struct cpu_state CPUs[MAX_CPUS];

void cpu_reset(cpu_state *cpu) {
    cpu->pc = read_word(cpu, RESET_VECTOR);
}
