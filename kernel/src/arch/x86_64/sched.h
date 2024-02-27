#pragma once
#include <arch/x86_64/sys/cpu.h>

/**
 * @brief Initializes the scheduler
 */
void x86_64_sched_init();

/**
 * @brief Sets up a new CPU for scheduling
 * @param release If false, cpu will block until stage is set to SCHED. If true, will set stage to SCHED
 */
[[noreturn]] void x86_64_sched_init_cpu(x86_64_cpu_t *cpu, bool release);

/**
 * @brief Switch to the next thread
 * @warning This essentially yields to the next thread, without the yield logic
 */
void x86_64_sched_next();