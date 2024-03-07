#pragma once
#include <stdint.h>
#include <drivers/acpi.h>

/**
 * @brief Initialize IOApic
 * @param apic_header MADT header
 */
void x86_64_ioapic_initialize(acpi_sdt_header_t *apic_header);

/**
 * @brief Map GSI to interrupt vector
 * @param gsi global system interrupt (IRQ)
 * @param apic_id local apic ID
 * @param low_polarity polarity; true = high active, false = low active
 * @param edge trigger mode; true = edge sensitive, false = level sensitive
 * @param vector interrupt vector
 */
void x86_64_ioapic_map_gsi(uint8_t gsi, uint8_t apic_id, bool low_polarity, bool trigger_mode, uint8_t vector);

/**
 * @brief Map legacy IRQ to interrupt vector
 * @param irq legacy IRQ
 * @param apic_id local apic ID
 * @param fallback_low_polarity fallback polarity; true = high active, false = low active
 * @param fallback_edge fallback trigger mode; true = edge sensitive, false = level sensitive
 * @param vector interrupt vector
 */
void x86_64_ioapic_map_legacy_irq(uint8_t irq, uint8_t apic_id, bool fallback_low_polarity, bool fallback_trigger_mode, uint8_t vector);