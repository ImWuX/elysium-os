#include "timer.h"
#include <cpu/isr.h>
#include <drivers/ports.h>
#include <memory/heap.h>

#define HW_CLOCK_FREQUENCY 1193182

static uint64_t g_ticks;
static uint16_t g_subticks;

static void timer_callback(cpu_register_t registers __attribute__((unused))) {
    g_subticks++;
    if(g_subticks >= 1000) return;
    g_subticks = 0;
    g_ticks++;
}

void initialize_timer() {
    register_interrupt_handler(IRQ_OFFSET, timer_callback);

    uint64_t divisor = HW_CLOCK_FREQUENCY / TIMER_FREQUENCY;
    uint8_t low = (uint8_t) (divisor & 0xFF);
    uint8_t high = (uint8_t) ((divisor >> 8) & 0xFF);

    outb(0x43, 0x36);
    outb(0x40, low);
    outb(0x40, high);
}

uint64_t get_time_s() {
    return g_ticks;
}

uint64_t get_time_ms() {
    return g_ticks * 1000 + g_subticks;
}