#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <arch/amd64/drivers/ps2.h>

typedef void (* ps2mouse_handler_t)(int16_t, int16_t, bool[3]);

/**
 * @brief Initialize PS2 mouse
 *
 * @param port PS2 port
 */
void ps2mouse_initialize(ps2_ports_t port);

/**
 * @brief Set PS2 mouse handler
 *
 * @param mouse_handler Mouse handler
 */
void ps2mouse_set_handler(ps2mouse_handler_t mouse_handler);