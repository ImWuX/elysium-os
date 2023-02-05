#ifndef DRIVERS_PS2_H
#define DRIVERS_PS2_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    PS2_PORT_ONE = 0,
    PS2_PORT_TWO = 1
} ps2_ports_t;

void ps2_initialize();
uint8_t ps2_status_read();
void ps2_status_write(uint8_t value);
bool ps2_port_write(ps2_ports_t port, uint8_t value);
uint8_t ps2_port_read(bool wait);

#endif