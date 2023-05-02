#ifndef ARCH_AMD64_IO_H
#define ARCH_AMD64_IO_H

#include <stdint.h>

typedef enum {
    IO_MSR_APIC_BASE   = 0x1B,
    IO_MSR_PAT         = 0x277,
    IO_MSR_EFER        = 0xC0000080,
    IO_MSR_STAR        = 0xC0000081,
    IO_MSR_LSTAR       = 0xC0000082,
    IO_MSR_CSTAR       = 0xC0000083,
    IO_MSR_SFMASK      = 0xC0000084
} io_msrs_t;

/**
 * @brief Port in (byte).
 * 
 * @param port Port number
 * @return Value in
 */
uint8_t io_port_inb(uint16_t port);

/**
 * @brief Port out (byte)
 * 
 * @param port Port number
 * @param value Value out
 */
void io_port_outb(uint16_t port, uint8_t value);

/**
 * @brief Port in (word).
 * 
 * @param port Port number
 * @return Value in
 */
uint32_t io_port_inl(uint16_t port);

/**
 * @brief Port out (word).
 * 
 * @param port Port number
 * @param value Value out
 */
void io_port_outl(uint16_t port, uint32_t value);

/**
 * @brief Read a machine specific register.
 * 
 * @param msr MSR number
 * @return Value in
 */
uint64_t io_msr_read(uint64_t msr);

/**
 * @brief Write to a machine specific register.
 * 
 * @param msr MSR number
 * @param value Value out
 */
void io_msr_write(uint64_t msr, uint64_t value);

#endif