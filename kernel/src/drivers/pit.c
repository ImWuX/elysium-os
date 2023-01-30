#include "pit.h"
#include <cpu/irq.h>
#include <drivers/ports.h>

#define CHANNEL0_DATA 0x40
#define CHANNEL1_DATA 0x41
#define CHANNEL2_DATA 0x42
#define CMD 0x43

#define HW_CLOCK_FREQUENCY 1193182

static uint64_t g_ticks;
static uint16_t g_subticks;

static void timer_callback(irq_cpu_register_t registers __attribute__((unused))) {
    if(++g_subticks < 1000) return;
    g_subticks = 0;
    g_ticks++;
}

void pit_initialize() {
    uint16_t divisor = HW_CLOCK_FREQUENCY / 1000;
    ports_outb(CMD, 0x36);
    ports_outb(CHANNEL0_DATA, divisor);
    ports_outb(CHANNEL0_DATA, divisor >> 8);

    irq_register_handler(32, timer_callback);
}

uint64_t pit_time_s() {
    return g_ticks;
}

uint64_t pit_time_ms() {
    return g_ticks * 1000 + g_subticks;
}