#include "pit.h"
#include <panic.h>
#include <cpu/interrupt.h>
#include <cpu/apic.h>
#include <drivers/ioapic.h>
#include <drivers/ports.h>
#include <memory/heap.h>

#define CHANNEL0_DATA 0x40
#define CHANNEL1_DATA 0x41
#define CHANNEL2_DATA 0x42
#define CMD 0x43

#define HW_CLOCK_FREQUENCY 1193182

static uint8_t g_interrupt_vector;
static uint64_t g_ticks;
static uint16_t g_subticks;
static pit_interval_t *g_head;

static void timer_handler(interrupt_frame_t *registers __attribute__((unused))) {
    apic_eoi(g_interrupt_vector);
    pit_interval_t *cur = g_head;
    while(cur) {
        if((g_ticks * 1000 + g_subticks + cur->offset) % cur->interval == 0) cur->cb();
        cur = cur->next;
    }

    if(++g_subticks < 1000) return;
    g_subticks = 0;
    g_ticks++;
}

void pit_initialize() {
    uint16_t divisor = HW_CLOCK_FREQUENCY / 1000;
    ports_outb(CMD, 0x36);
    ports_outb(CHANNEL0_DATA, divisor);
    ports_outb(CHANNEL0_DATA, divisor >> 8);

    int vector = interrupt_request(INTERRUPT_PRIORITY_TIMER, timer_handler);
    if(vector < 0) panic("PIT", "Failed to acquire interrupt vector");
    g_interrupt_vector = vector;
    ioapic_map_legacy_irq(0, apic_id(), false, true, g_interrupt_vector);
}

void pit_interval(int interval, void (* cb)()) {
    pit_interval_t *new = heap_alloc(sizeof(pit_interval_t));
    new->interval = interval;
    new->offset = (g_ticks * 1000 + g_subticks) % interval;
    new->cb = cb;
    new->next = g_head;
    g_head = new;
}

uint64_t pit_time_s() {
    return g_ticks;
}

uint64_t pit_time_ms() {
    return g_ticks * 1000 + g_subticks;
}