#include "apic.h"
#include <stdio.h>
#include <cpu/msr.h>
#include <memory/vmm.h>
#include <memory/hhdm.h>

#define APIC_BASE_MSR_ADDRESS 0xFFFFFFFFFF000
#define APIC_BASE_MSR_AE (1 << 11)
#define APIC_BASE_MSR_EXTD (1 << 10)
#define APIC_BASE_MSR_BSC (1 << 8)

#define LAPIC_ID 0x2
#define LAPIC_ID_AID(id) (((id) >> 24) & 0xFF)
#define LAPIC_SPURIOUS_INTERRUPT 0xF
#define LAPIC_SPURIOUS_INTERRUPT_ENABLE (1 << 8)
#define LAPIC_EOI 0xB
#define LAPIC_IN_SERVICE 0x10

static volatile uint32_t *g_lapic;

static void lapic_write(uint32_t index, uint32_t data) {
    g_lapic[index * 4] = data;
}

static uint32_t lapic_read(uint32_t index) {
    return g_lapic[index * 4];
}

void apic_initialize() {
    g_lapic = (uint32_t *) HHDM(msr_get(MSR_APIC_BASE) & APIC_BASE_MSR_ADDRESS);
    lapic_write(LAPIC_SPURIOUS_INTERRUPT, 0xFF | LAPIC_SPURIOUS_INTERRUPT_ENABLE);
}

void apic_eoi(uint8_t interrupt_vector) {
    uint8_t register_index = interrupt_vector / 32;
    uint8_t index = interrupt_vector % 32;
    if(lapic_read(LAPIC_IN_SERVICE + register_index) & (1 << index)) lapic_write(LAPIC_EOI, 1);
}

uint8_t apic_id() {
    return LAPIC_ID_AID(lapic_read(LAPIC_ID));
}