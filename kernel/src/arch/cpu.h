#pragma once

/**
 * @brief Relaxes the CPU (ex. on amd64 this will emit the `pause` instruction)
 */
void arch_cpu_relax();