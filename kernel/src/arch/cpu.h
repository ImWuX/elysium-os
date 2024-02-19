#pragma once

/**
 * @brief "Relaxes" the CPU.
 */
void arch_cpu_relax();

/**
 * @brief Halts the CPU.
 */
[[noreturn]] void arch_cpu_halt();