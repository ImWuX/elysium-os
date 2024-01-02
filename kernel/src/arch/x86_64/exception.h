#pragma once
#include <arch/x86_64/interrupt.h>

/**
 * @brief Panic stub for exceptions.
 * @param frame interrupt frame
 */
[[noreturn]] void exception_unhandled(interrupt_frame_t *frame);