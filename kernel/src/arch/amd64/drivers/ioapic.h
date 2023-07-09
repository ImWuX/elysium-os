#ifndef ARCH_AMD64_DRIVERS_IOAPIC_H
#define ARCH_AMD64_DRIVERS_IOAPIC_H

#include <stdint.h>
#include <stdbool.h>
#include <drivers/acpi.h>

/**
 * @brief Initialize IOApic
 *
 * @param apic_header MADT header
 */
void ioapic_initialize(acpi_sdt_header_t *apic_header);

/**
 * @brief Map GSI to interrupt vector
 *
 * @param gsi Global system interrupt (IRQ)
 * @param apic_id Local apic ID
 * @param low_polarity Polarity; true = high active, false = low active
 * @param edge Trigger mode; true = edge sensitive, false = level sensitive
 * @param vector Interrupt vector
 */
void ioapic_map_gsi(uint8_t gsi, uint8_t apic_id, bool low_polarity, bool trigger_mode, uint8_t vector);

/**
 * @brief Map legacy IRQ to interrupt vector
 *
 * @param irq Legacy IRQ
 * @param apic_id Local apic ID
 * @param fallback_low_polarity Fallback polarity; true = high active, false = low active
 * @param fallback_edge Fallback trigger mode; true = edge sensitive, false = level sensitive
 * @param vector Interrupt vector
 */
void ioapic_map_legacy_irq(uint8_t irq, uint8_t apic_id, bool fallback_low_polarity, bool fallback_trigger_mode, uint8_t vector);

#endif