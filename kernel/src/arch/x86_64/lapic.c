#include "lapic.h"
#include <memory/hhdm.h>
#include <arch/x86_64/msr.h>

#define BASE_MASK 0xFFFFFFFFFF000

#define REG_ID 0x20
#define REG_SPURIOUS 0xF0
#define REG_EOI 0xB0
#define REG_IN_SERVICE_BASE 0x100

static inline void lapic_write(uint32_t reg, uint32_t data) {
    *(volatile uint32_t *) HHDM((msr_read(MSR_APIC_BASE) & BASE_MASK) + reg) = data;
}

static inline uint32_t lapic_read(uint32_t reg) {
    return *(volatile uint32_t *) HHDM((msr_read(MSR_APIC_BASE) & BASE_MASK) + reg);
}

void lapic_initialize() {
    lapic_write(REG_SPURIOUS, 0xFF | (1 << 8));
}

void lapic_eoi(uint8_t interrupt_vector) {
    if(lapic_read(REG_IN_SERVICE_BASE + interrupt_vector / 32 * 0x10) & (1 << (interrupt_vector % 32))) lapic_write(REG_EOI, 0);
}

uint8_t lapic_id() {
    return (uint8_t) (lapic_read(REG_ID) >> 24);
}