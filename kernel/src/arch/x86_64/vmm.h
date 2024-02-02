#pragma once
#include <arch/x86_64/interrupt.h>

/**
 * @brief Handles page faults and passes them to the arch agnostic handler.
 * @param frame interrupt frame
 */
void x86_64_vmm_page_fault_handler(x86_64_interrupt_frame_t *frame);