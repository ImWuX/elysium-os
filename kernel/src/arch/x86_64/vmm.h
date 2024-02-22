#pragma once
#include <memory/vmm.h>
#include <arch/x86_64/interrupt.h>

/**
 * @brief Initialize the vmm
 */
vmm_address_space_t *x86_64_vmm_init();

/**
 * @brief Handles page faults and passes them to the arch agnostic handler
 */
void x86_64_vmm_page_fault_handler(x86_64_interrupt_frame_t *frame);