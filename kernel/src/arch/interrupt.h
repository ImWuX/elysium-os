#pragma once

typedef enum {
    ARCH_INTERRUPT_IPL_NORMAL,
    ARCH_INTERRUPT_IPL_IPC,
    ARCH_INTERRUPT_IPL_CRITICAL
} arch_interrupt_ipl_t;

/**
 * @brief Set the current IPL level.
 * @param ipl new ipl
 * @returns old ipl
 */
arch_interrupt_ipl_t arch_interrupt_ipl(arch_interrupt_ipl_t ipl);