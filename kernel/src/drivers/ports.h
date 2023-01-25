#ifndef DRIVERS_PORTS_H
#define DRIVERS_PORTS_H

#include <stdint.h>

uint8_t ports_inb(uint16_t port);
void ports_outb(uint16_t port, uint8_t value);
uint32_t ports_inl(uint16_t port);
void ports_outl(uint16_t port, uint32_t value);

#endif