#pragma once
#include <arch/x86_64/interrupt.h>

/**
 * @brief Panic stub for exceptions.
 * @param frame interrupt frame
 */
[[noreturn]] void x86_64_exception_unhandled(x86_64_interrupt_frame_t *frame);