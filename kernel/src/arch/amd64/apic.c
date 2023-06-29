#include "apic.h"
#include <stdio.h>
#include <arch/amd64/msr.h>
#include <memory/vmm.h>
#include <memory/hhdm.h>

#define BASE_MASK 0xFFFFFFFFFF000

#define REG_ID 0x20
#define REG_SPURIOUS 0xF0
#define REG_EOI 0xB0
#define REG_IN_SERVICE_BASE 0x100

static void lapic_write(uint32_t offset, uint32_t data) {
    *(uint32_t *) HHDM((msr_read(MSR_APIC_BASE) & BASE_MASK) + offset) = data;
}

static uint32_t lapic_read(uint32_t offset) {
    return *(uint32_t *) HHDM((msr_read(MSR_APIC_BASE) & BASE_MASK) + offset);
}

void apic_initialize() {
    lapic_write(REG_SPURIOUS, 0xFF | (1 << 8));
}

void apic_eoi(uint8_t interrupt_vector) {
    if(lapic_read(REG_IN_SERVICE_BASE + interrupt_vector / 32) & (1 << (interrupt_vector % 32))) lapic_write(REG_EOI, 1);
}

uint8_t apic_id() {
    return (uint8_t) (lapic_read(REG_ID) << 24);
}