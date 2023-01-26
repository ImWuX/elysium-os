#include "pit.h"
#include <cpu/irq.h>
#include <drivers/ports.h>

#define HW_CLOCK_FREQUENCY 1193182

static uint64_t g_ticks;
static uint16_t g_subticks;

static void timer_callback(irq_cpu_register_t registers __attribute__((unused))) {
    g_subticks++;
    if(g_subticks >= 1000) return;
    g_subticks = 0;
    g_ticks++;
}

void pit_initialize() {
    irq_register_handler(32, timer_callback);

    uint64_t divisor = HW_CLOCK_FREQUENCY / 1000;
    uint8_t low = (uint8_t) (divisor & 0xFF);
    uint8_t high = (uint8_t) ((divisor >> 8) & 0xFF);

    ports_outb(0x43, 0x36);
    ports_outb(0x40, low);
    ports_outb(0x40, high);
}

uint64_t pit_time_s() {
    return g_ticks;
}

uint64_t pit_time_ms() {
    return g_ticks * 1000 + g_subticks;
}