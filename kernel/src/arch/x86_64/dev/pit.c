#include "pit.h"
#include <arch/x86_64/port.h>

#define CHANNEL0_DATA 0x40
#define CHANNEL1_DATA 0x41
#define CHANNEL2_DATA 0x42
#define CMD 0x43

void x86_64_pit_initialize(uint16_t divisor) {
    x86_64_port_outb(CMD, 0x34);
    x86_64_port_outb(CHANNEL0_DATA, (uint8_t) divisor);
    x86_64_port_outb(CHANNEL0_DATA, (uint8_t) (divisor >> 8));
}

uint16_t x86_64_pit_count() {
    x86_64_port_outb(CMD, 0);
    uint16_t low = x86_64_port_inb(CHANNEL0_DATA);
    uint16_t high = x86_64_port_inb(CHANNEL0_DATA);
    return low | (high << 8);
}