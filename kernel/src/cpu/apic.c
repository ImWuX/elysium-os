#include "apic.h"
#include <stdio.h>
#include <cpu/msr.h>
#include <memory/vmm.h>

#define IA32_APIC_BASE_MSR 0x1B

#define PIC_8259_MASTER_CMD 0x20
#define PIC_8259_MASTER_DATA 0x21
#define PIC_8259_SLAVE_CMD 0xA0
#define PIC_8259_SLAVE_DATA 0xA1

static void disable_8259_pic() {
    outb(PIC_8259_MASTER_CMD, 0x11);
    outb(PIC_8259_SLAVE_CMD, 0x11);
    outb(PIC_8259_MASTER_DATA, 0x20);
    outb(PIC_8259_SLAVE_DATA, 0x28);
    outb(PIC_8259_MASTER_DATA, 0x04);
    outb(PIC_8259_SLAVE_DATA, 0x02);
    outb(PIC_8259_MASTER_DATA, 0x01);
    outb(PIC_8259_SLAVE_DATA, 0x01);
    outb(PIC_8259_MASTER_DATA, 0xFF);
    outb(PIC_8259_SLAVE_DATA, 0xFF);
}

void initialize_apic(sdt_header_t *apic_header) {
    disable_8259_pic();

    madt_header_t *madt = (madt_header_t *) apic_header;
    uint64_t lapic_address = madt->local_apic_address;
    uint64_t ioapic_address = 0;
    uint8_t lapic_ids[256];
    uint8_t core_count = 0;

    int nbytes = madt->sdt_header.length - sizeof(madt_header_t);
    madt_record_t *current_record = (madt_record_t *) ((uint64_t) madt + sizeof(madt_header_t));
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
        }
        nbytes -= current_record->length;
        current_record = (madt_record_t *) ((uint64_t) current_record + current_record->length);
    }

    // uint32_t *lapic_registers = get_hhdm_address(lapic_address);
    // uint32_t value = lapic_registers[7];
    // value |= 1 << 8;
    // lapic_registers[7] = value;

    // printf("LAPIC Address: %x\nIOAPIC Address: %x\nCore count: %i\n", lapic_address, ioapic_address, core_count);

    // Atleast get the APIC address. Either from MADT or the MSR (or both? atleast MSR? ig)
    //  Possibly also set the address to a definitely available memory location and map it.
    // Next, we want to disable the 8259 PIC
    // Then we want to take the APIC out of 8259 emulation mode
}