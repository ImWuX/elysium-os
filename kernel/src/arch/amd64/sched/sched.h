#pragma once
#include <arch/amd64/cpu.h>

/**
 * @brief Globally initializes the scheduler
 */
void sched_init();

/**
 * @brief Sets up a new CPU for scheduling
 *
 * @param cpu CPU local data
 */
[[noreturn]] void sched_init_cpu(arch_cpu_t *cpu);