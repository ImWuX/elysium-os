#ifndef ARCH_AMD64_IO_H
#define ARCH_AMD64_IO_H

#include <stdint.h>

/**
 * @brief Port in (byte).
 * 
 * @param port Port number
 * @return Value in
 */
uint8_t port_inb(uint16_t port);

/**
 * @brief Port out (byte)
 * 
 * @param port Port number
 * @param value Value out
 */
void port_outb(uint16_t port, uint8_t value);

/**
 * @brief Port in (word).
 * 
 * @param port Port number
 * @return Value in
 */
uint32_t port_inl(uint16_t port);

/**
 * @brief Port out (word).
 * 
 * @param port Port number
 * @param value Value out
 */
void port_outl(uint16_t port, uint32_t value);

#endif