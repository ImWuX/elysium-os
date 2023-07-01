#ifndef ARCH_AMD64_DRIVERS_IOAPIC_H
#define ARCH_AMD64_DRIVERS_IOAPIC_H

#include <stdint.h>
#include <stdbool.h>
#include <drivers/acpi.h>

void ioapic_initialize(acpi_sdt_header_t *apic_header);
void ioapic_map_gsi(uint8_t gsi, uint8_t apic_id, bool low_polarity, bool edge, uint8_t vector);
void ioapic_map_legacy_irq(uint8_t irq, uint8_t apic_id, bool fallback_low_polarity, bool fallback_edge, uint8_t vector);

#endif