#ifndef ARCH_AMD64_DRIVERS_PS2KB_H
#define ARCH_AMD64_DRIVERS_PS2KB_H

#include <stdint.h>
#include <arch/amd64/drivers/ps2.h>

typedef void (* ps2kb_handler_t)(uint8_t);

/**
 * @brief Initialize PS2 keyboard
 *
 * @param port PS2 port
 */
void ps2kb_initialize(ps2_ports_t port);

/**
 * @brief Set PS2 keyboard handler
 *
 * @param keyboard_handler Keyboard handler
 */
void ps2kb_set_handler(ps2kb_handler_t keyboard_handler);

#endif