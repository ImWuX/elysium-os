#pragma once
#include <sys/cpu.h>

/**
 * @brief "Relaxes" the CPU.
 */
void arch_cpu_relax();

/**
 * @brief Halts the CPU.
 */
[[noreturn]] void arch_cpu_halt();

/**
 * @brief Get the current CPU local.
 */
cpu_t *arch_cpu_current();