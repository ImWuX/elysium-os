#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef enum {
    X86_64_PS2_PORT_ONE = 0,
    X86_64_PS2_PORT_TWO = 1
} x86_64_ps2_port_t;

typedef enum {
    X86_64_PS2_PORT_ONE_IRQ = 1,
    X86_64_PS2_PORT_TWO_IRQ = 12,
} x86_64_ps2_legacy_irq_t;

/**
 * @brief Initialize the PS2 controllers
 */
void x86_64_ps2_initialize();

/**
 * @brief Enable a PS2 port
 */
void x86_64_ps2_port_enable(x86_64_ps2_port_t port);

/**
 * @brief Read from the PS2 controllers output buffer
 * @param wait wait for ready status
 */
uint8_t x86_64_ps2_port_read(bool wait);

/**
 * @brief Write to a PS2 port
 * @returns false = write failed, true = write success
 */
bool x86_64_ps2_port_write(x86_64_ps2_port_t port, uint8_t value);