#pragma once
#include <arch/amd64/interrupt.h>

/**
 * @brief Panic stub for exceptions
 *
 * @param frame Interrupt frame
 */
[[noreturn]] void exception_unhandled(interrupt_frame_t *frame);