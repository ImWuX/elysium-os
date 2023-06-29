#ifndef ARCH_AMD64_APIC_H
#define ARCH_AMD64_APIC_H

#include <stdint.h>

/**
 * @brief Initialize and enable the local apic for the current core
 */
void apic_initialize();

/**
 * @brief Signal an end of interrupt
 * 
 * @param interrupt_vector Interrupt vector
 */
void apic_eoi(uint8_t interrupt_vector);

/**
 * @brief Get the local apic id of the current cores local apic
 * 
 * @return Local apic ID
 */
uint8_t apic_id();

#endif