#pragma once
#include <stdint.h>
#include <arch/x86_64/dev/ps2.h>

typedef void (* x86_64_ps2kb_handler_t)(uint8_t);

/**
 * @brief Initialize PS2 keyboard
 */
void x86_64_ps2kb_initialize(x86_64_ps2_port_t port);

/**
 * @brief Set PS2 keyboard handler
 */
void x86_64_ps2kb_set_handler(x86_64_ps2kb_handler_t keyboard_handler);