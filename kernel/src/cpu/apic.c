#include "apic.h"
#include <stdio.h>
#include <cpu/msr.h>
#include <cpu/idt.h>
#include <memory/vmm.h>
#include <memory/hhdm.h>

#define APIC_BASE_MSR_ADDRESS 0xFFFFFFFFFF000
#define APIC_BASE_MSR_AE (1 << 11)
#define APIC_BASE_MSR_EXTD (1 << 10)
#define APIC_BASE_MSR_BSC (1 << 8)

#define IOAPIC_VER 0x1
#define IOAPIC_VER_MAX_REDIRECTION_ENTRY(val) (((val) >> 16) & 0xFF)

#define LAPIC_ID 0x2
#define LAPIC_SPURIOUS_INTERRUPT 0xF
#define LAPIC_SPURIOUS_INTERRUPT_ENABLE (1 << 8)
#define LAPIC_EOI 0xB
#define LAPIC_IN_SERVICE 0x10

static volatile uint32_t *g_lapic;
static volatile uint32_t *g_ioapic;

static apic_legacy_irq_translation_t g_legacy_irq_map[16] = {
    {0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0}, {5, 0},
    {6, 0}, {7, 0}, {8, 0}, {9, 0}, {10, 0},
    {11, 0}, {12, 0}, {13, 0}, {14, 0}, {15, 0}
};

static void lapic_write(uint32_t index, uint32_t data) {
    g_lapic[index * 4] = data;
}

static uint32_t lapic_read(uint32_t index) {
    return g_lapic[index * 4];
}

static void ioapic_write(uint32_t index, uint32_t data) {
    g_ioapic[0] = index & 0xFF;
    g_ioapic[4] = data;
}

static uint32_t ioapic_read(uint32_t index) {
    g_ioapic[0] = index & 0xFF;
    return g_ioapic[4];
}

static void ioapic_set_irq(uint8_t irq, uint64_t apic_id, uint8_t vector) {
    uint32_t low_irq = 0x10 + irq * 2;
    uint32_t high_irq = low_irq + 1;

    uint32_t high_data = ioapic_read(high_irq);
    high_data &= ~0xFF000000;
    high_data |= apic_id << 24;
    ioapic_write(high_irq, high_data);

    uint32_t low_data = ioapic_read(low_irq);
    low_data &= ~(1 << 16);
    low_data &= ~(1 << 11);
    low_data &= ~0x700;
    low_data &= ~0xFF;
    low_data |= vector;
    ioapic_write(low_irq, low_data);
}

void apic_initialize(acpi_sdt_header_t *apic_header) {
    apic_madt_header_t *madt = (apic_madt_header_t *) apic_header;
    uintptr_t ioapic_address = 0;
    uint8_t lapic_ids[256] __attribute__((unused)); //TODO: attribute unused
    uint8_t core_count = 0;

    uint32_t nbytes = madt->sdt_header.length - sizeof(apic_madt_header_t);
    apic_madt_record_t *current_record = (apic_madt_record_t *) ((uintptr_t) madt + sizeof(apic_madt_header_t));
    while(nbytes > 0) {
        switch(current_record->type) {
            case LAPIC:
                apic_madt_record_lapic_t *lapic_record = (apic_madt_record_lapic_t *) current_record;
                lapic_ids[core_count++] = lapic_record->apic_id;
                break;
            case IOAPIC:
                apic_madt_record_ioapic_t *ioapic_record = (apic_madt_record_ioapic_t *) current_record;
                ioapic_address = ioapic_record->ioapic_address;

                break;
            case SOURCE_OVERRIDE:
                apic_madt_record_source_override_t *override_record = (apic_madt_record_source_override_t *) current_record;
                g_legacy_irq_map[override_record->irq_source].gsi = override_record->global_system_interrupt;
                g_legacy_irq_map[override_record->irq_source].flags = override_record->flags;
                break;
            case NMI:
                apic_madt_record_nmi_t *nmi_record = (apic_madt_record_nmi_t *) current_record;
                break;
        }
        nbytes -= current_record->length;
        current_record = (apic_madt_record_t *) ((uintptr_t) current_record + current_record->length);
    }

    g_lapic = (uint32_t *) HHDM(msr_get(MSR_APIC_BASE) & APIC_BASE_MSR_ADDRESS);
    g_ioapic = (uint32_t *) HHDM(ioapic_address);

    lapic_write(LAPIC_SPURIOUS_INTERRUPT, 0xFF | LAPIC_SPURIOUS_INTERRUPT_ENABLE);
    // uint32_t val = ioapic_read(IOAPIC_VER);
}

// void apic_map_legacy_irq(uint8_t irq, uint8_t gsi) {

// }

void apic_eoi(uint8_t interrupt_vector) {
    uint8_t register_index = interrupt_vector / 32;
    uint8_t index = interrupt_vector % 32;
    if(lapic_read(LAPIC_IN_SERVICE + register_index) & (1 << index)) lapic_write(LAPIC_EOI, 1);
}