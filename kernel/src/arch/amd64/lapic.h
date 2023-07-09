#ifndef ARCH_AMD64_LAPIC_H
#define ARCH_AMD64_LAPIC_H

#include <stdint.h>

/**
 * @brief Initialize and enable the local apic for the current core
 */
void lapic_initialize();

/**
 * @brief Polls the local apic timer
 * @warning This function blocks until polling is done
 *
 * @param ticks Ticks to poll
 */
void lapic_timer_poll(uint32_t ticks);

/**
 * @brief Perform a oneshot event using the apic timer
 *
 * @param vector Interrupt vector
 * @param us Time in microseconds
 */
void lapic_timer_oneshot(uint8_t vector, uint64_t us);

/**
 * @brief Stop the local apic timer
 */
void lapic_timer_stop();

/**
 * @brief Signal an end of interrupt
 *
 * @param interrupt_vector Interrupt vector
 */
void lapic_eoi(uint8_t interrupt_vector);

/**
 * @brief Get the local apic id of the current cores local apic
 *
 * @return Local apic ID
 */
uint8_t lapic_id();

#endif