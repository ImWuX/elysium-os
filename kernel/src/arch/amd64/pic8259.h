#ifndef ARCH_AMD64_PIC8259_H
#define ARCH_AMD64_PIC8259_H

#include <stdint.h>

/**
 * @brief Remap the legacy PIC irqs.
 */
void pic8259_remap();

/**
 * @brief Disable the legacy PIC (mask all irqs).
 */
void pic8259_disable();

/**
 * @brief Issue an end of interrupt to the legacy PIC.
 * 
 * @param interrupt_vector Vector of the interrupt
 */
void pic8259_eoi(uint8_t interrupt_vector);

#endif