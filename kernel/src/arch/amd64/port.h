#pragma once
#include <stdint.h>

/**
 * @brief Port in (byte)
 *
 * @param port Port number
 * @return Value in
 */
static inline uint8_t port_inb(uint16_t port) {
    uint8_t result;
    asm volatile("inb %1, %0" : "=a" (result) : "Nd" (port));
    return result;
}

/**
 * @brief Port out (byte)
 *
 * @param port Port number
 * @param value Value out
 */
static inline void port_outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a" (value), "Nd" (port));
}

/**
 * @brief Port in (word)
 *
 * @param port Port number
 * @return Value in
 */
static inline uint32_t port_inl(uint16_t port) {
    uint32_t result;
    asm volatile("inl %1, %0" : "=a" (result) : "Nd" (port));
    return result;
}

/**
 * @brief Port out (word)
 *
 * @param port Port number
 * @param value Value out
 */
static inline void port_outl(uint16_t port, uint32_t value) {
    asm volatile("outl %0, %1" : : "a" (value), "Nd" (port));
}