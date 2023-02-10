#ifndef DRIVERS_MOUSE_H
#define DRIVERS_MOUSE_H

#include <stdint.h>
#include <stdbool.h>
#include <drivers/ps2.h>

typedef void (* mouse_handler_t)(int16_t, int16_t, bool[3]);

void mouse_initialize(ps2_ports_t port);
void mouse_set_handler(mouse_handler_t mouse_handler);

#endif