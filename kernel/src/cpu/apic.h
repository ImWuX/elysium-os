#ifndef CPU_APIC_H
#define CPU_APIC_H

#include <stdint.h>
#include <drivers/acpi.h>

typedef struct {
    acpi_sdt_header_t sdt_header;
    uint32_t local_apic_address;
    uint32_t flags;
} __attribute__((packed)) apic_madt_header_t;

typedef enum {
    LAPIC = 0,
    IOAPIC,
    SOURCE_OVERRIDE,
    NMI_SOURCE,
    NMI,
    LAPIC_ADDRESS,
    LX2APIC = 9
} apic_madt_record_type_t;

typedef struct {
    uint8_t type;
    uint8_t length;
} __attribute__((packed)) apic_madt_record_t;

typedef struct {
    apic_madt_record_t base;
    uint8_t acpi_processor_id;
    uint8_t apic_id;
    uint32_t flags;
} __attribute__((packed)) apic_madt_record_lapic_t;

typedef struct {
    apic_madt_record_t base;
    uint8_t ioapic_id;
    uint8_t rsv0;
    uint32_t ioapic_address;
    uint32_t global_system_interrupt_base;
} __attribute__((packed)) apic_madt_record_ioapic_t;

typedef struct {
    apic_madt_record_t base;
    uint16_t rsv0;
    uint64_t lapic_address;
} __attribute__((packed)) apic_madt_record_lapic_address_t;

typedef struct {
    apic_madt_record_t base;
    uint8_t bus_source;
    uint8_t irq_source;
    uint32_t global_system_interrupt;
    uint16_t flags;
} __attribute__((packed)) apic_madt_record_source_override_t;

typedef struct {
    apic_madt_record_ioapic_t base;
    uint8_t acpi_processor_id;
    uint16_t flags;
    uint8_t lint;
} __attribute__((packed)) apic_madt_record_nmi_t;

typedef struct {
    uint8_t gsi;
    uint16_t flags;
} apic_legacy_irq_translation_t;

void apic_initialize(acpi_sdt_header_t *apic_header);
void apic_eoi(uint8_t interrupt_vector);

#endif