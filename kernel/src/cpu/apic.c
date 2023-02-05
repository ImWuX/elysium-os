#include "apic.h"
#include <stdio.h>
#include <cpu/idt.h>
#include <memory/vmm.h>
#include <memory/hhdm.h>

uint32_t *g_lapic;
uint32_t *g_ioapic;

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
    madt_header_t *madt = (madt_header_t *) apic_header;
    uintptr_t lapic_address = madt->local_apic_address;
    uintptr_t ioapic_address = 0;
    uint8_t lapic_ids[256] __attribute__((unused)); //TODO: attribute unused
    uint8_t core_count = 0;

    int nbytes = madt->sdt_header.length - sizeof(madt_header_t);
    madt_record_t *current_record = (madt_record_t *) ((uintptr_t) madt + sizeof(madt_header_t));
    while(nbytes > 0) {
        switch(current_record->type) {
            case LAPIC:
                madt_record_lapic_t *lapic_record = (madt_record_lapic_t *) current_record;
                lapic_ids[core_count++] = lapic_record->apic_id;
                break;
            case IOAPIC:
                madt_record_ioapic_t *ioapic_record = (madt_record_ioapic_t *) current_record;
                ioapic_address = ioapic_record->ioapic_address;
                break;
            case LAPIC_ADDRESS:
                lapic_address = ((madt_record_lapic_address_t *) current_record)->lapic_address;
                break;
            case SOURCE_OVERRIDE:
                madt_record_source_override_t *override_record = (madt_record_source_override_t *) current_record;
                // TODO: Parse source overrides
                break;
            case NMI:
                madt_record_nmi_t *nmi_record = (madt_record_nmi_t *) current_record;
                break;
        }
        nbytes -= current_record->length;
        current_record = (madt_record_t *) ((uintptr_t) current_record + current_record->length);
    }

    g_lapic = (uint32_t *) HHDM(lapic_address);
    g_ioapic = (uint32_t *) HHDM(ioapic_address);
    // vmm_map((void *) lapic_address, (void *) g_lapic); //TODO: Use hhdm_map once the VMM handles huge pages
    // vmm_map((void *) ioapic_address, (void *) g_ioapic);

    lapic_write(0xF, 0x100 | 0xFF);

    ioapic_set_irq(2, lapic_read(2), 32);
    ioapic_set_irq(1, lapic_read(2), 32 + 6);
    ioapic_set_irq(12, lapic_read(2), 32 + 7);
    ioapic_set_irq(8, lapic_read(2), 32 + 8);
}

void apic_eoi(uint8_t interrupt_vector) {
    if(g_lapic == 0) // TODO: Shouldn't be needed after full implementation
        return;

    uint8_t register_index = interrupt_vector / 32;
    uint8_t index = interrupt_vector % 32;

    if(lapic_read(0x10 + register_index) & (1 << index))
        lapic_write(0xB, 1);
}