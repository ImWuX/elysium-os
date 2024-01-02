#pragma once
#include <stdint.h>

/**
 * @brief Initialize and enable the local apic for the current core.
 */
void lapic_initialize();

/**
 * @brief Get the local apic id of the current core.
 * @return local apic id
 */
uint8_t lapic_id();

/**
 * @brief Issue an end of interrupt.
 * @param interrupt_vector
 */
void lapic_eoi(uint8_t interrupt_vector);