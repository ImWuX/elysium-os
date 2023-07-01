#ifndef ARCH_AMD64_DRIVERS_PS2_H
#define ARCH_AMD64_DRIVERS_PS2_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    PS2_PORT_ONE = 0,
    PS2_PORT_TWO = 1
} ps2_ports_t;

typedef enum {
    PS2_PORT_ONE_IRQ = 1,
    PS2_PORT_TWO_IRQ = 12,
} ps2_legacy_irqs_t;

void ps2_initialize();
void ps2_port_enable(ps2_ports_t port);
uint8_t ps2_port_read(bool wait);
bool ps2_port_write(ps2_ports_t port, uint8_t value);

#endif