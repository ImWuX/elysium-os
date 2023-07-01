#ifndef ARCH_AMD64_DRIVERS_PS2KB_H
#define ARCH_AMD64_DRIVERS_PS2KB_H

#include <stdint.h>
#include <arch/amd64/drivers/ps2.h>

typedef void (* ps2kb_handler_t)(uint8_t);

void ps2kb_initialize(ps2_ports_t port);
void ps2kb_set_handler(ps2kb_handler_t keyboard_handler);

#endif