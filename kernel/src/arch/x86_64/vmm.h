#pragma once
#include <arch/x86_64/interrupt.h>

/**
 * @brief Handles page faults and passes them to the arch agnostic handler.
 * @param frame interrupt frame
 */
void vmm_page_fault_handler(interrupt_frame_t *frame);