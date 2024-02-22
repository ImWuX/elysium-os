#pragma once
#include <arch/x86_64/cpu.h>

/**
 * @brief Initializes the scheduler
 */
void x86_64_sched_init();

/**
 * @brief Sets up a new CPU for scheduling
 */
[[noreturn]] void x86_64_sched_init_cpu(x86_64_cpu_t *cpu);

/**
 * @brief Switch to the next thread
 * @warning This essentially yields to the next thread, without the yield logic
 */
void x86_64_sched_next();