#include "lapic.h"
#include <lib/kprint.h>
#include <memory/vmm.h>
#include <memory/hhdm.h>
#include <arch/sched.h>
#include <arch/amd64/cpu.h>
#include <arch/amd64/msr.h>

#define BASE_MASK 0xFFFFFFFFFF000

#define REG_ID 0x20
#define REG_SPURIOUS 0xF0
#define REG_EOI 0xB0
#define REG_IN_SERVICE_BASE 0x100
#define REG_LVT_TIMER 0x320
#define REG_TIMER_DIV 0x3E0
#define REG_TIMER_INITIAL_COUNT 0x380
#define REG_TIMER_CURRENT_COUNT 0x390

static inline void lapic_write(uint32_t reg, uint32_t data) {
    *(volatile uint32_t *) HHDM((msr_read(MSR_APIC_BASE) & BASE_MASK) + reg) = data;
}

static inline uint32_t lapic_read(uint32_t reg) {
    return *(volatile uint32_t *) HHDM((msr_read(MSR_APIC_BASE) & BASE_MASK) + reg);
}

void lapic_initialize() {
    lapic_write(REG_SPURIOUS, 0xFF | (1 << 8));
}

void lapic_timer_poll(uint32_t ticks) {
    lapic_timer_stop();
    lapic_write(REG_LVT_TIMER, (1 << 16) | 0xFF);
    lapic_write(REG_TIMER_DIV, 0);
    lapic_write(REG_TIMER_INITIAL_COUNT, ticks);
    while(lapic_read(REG_TIMER_CURRENT_COUNT) != 0);
    lapic_timer_stop();
}

void lapic_timer_oneshot(uint8_t vector, uint64_t us) {
    lapic_timer_stop();
    lapic_write(REG_LVT_TIMER, vector);
    lapic_write(REG_TIMER_DIV, 0);
    lapic_write(REG_TIMER_INITIAL_COUNT, us * (ARCH_CPU(arch_sched_current_thread()->cpu)->lapic_timer_frequency / 1'000'000));
}

void lapic_timer_stop() {
    lapic_write(REG_TIMER_INITIAL_COUNT, 0);
    lapic_write(REG_LVT_TIMER, (1 << 16));
}

void lapic_eoi(uint8_t interrupt_vector) {
    if(lapic_read(REG_IN_SERVICE_BASE + interrupt_vector / 32 * 0x10) & (1 << (interrupt_vector % 32))) lapic_write(REG_EOI, 0);
}

uint8_t lapic_id() {
    return (uint8_t) (lapic_read(REG_ID) >> 24);
}