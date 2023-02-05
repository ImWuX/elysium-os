#ifndef DRIVERS_KEYBOARD_H
#define DRIVERS_KEYBOARD_H

#include <stdint.h>
#include <drivers/ps2.h>

typedef void (* keyboard_handler_t)(uint8_t);

void keyboard_initialize(ps2_ports_t port);
void keyboard_set_handler(keyboard_handler_t keyboard_handler);

#endif