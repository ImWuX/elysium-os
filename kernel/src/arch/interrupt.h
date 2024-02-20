#pragma once
#include <sys/ipl.h>

/**
 * @brief Set the IPL level
 */
void arch_interrupt_set_ipl(ipl_t ipl);

/**
 * @brief Get the IPL level
 */
ipl_t arch_interrupt_get_ipl();