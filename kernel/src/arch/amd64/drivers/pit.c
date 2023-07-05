#include "pit.h"
#include <arch/amd64/port.h>

#define CHANNEL0_DATA 0x40
#define CHANNEL1_DATA 0x41
#define CHANNEL2_DATA 0x42
#define CMD 0x43

void pit_initialize(uint16_t divisor) {
    port_outb(CMD, 0x34);
    port_outb(CHANNEL0_DATA, (uint8_t) divisor);
    port_outb(CHANNEL0_DATA, (uint8_t) (divisor >> 8));
}

uint16_t pit_count() {
    port_outb(CMD, 0);
    uint16_t low = port_inb(CHANNEL0_DATA);
    uint16_t high = port_inb(CHANNEL0_DATA);
    return low | (high << 8);
}