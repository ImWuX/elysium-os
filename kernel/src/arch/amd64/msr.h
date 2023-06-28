#ifndef ARCH_AMD64_MSR_H
#define ARCH_AMD64_MSR_H

#include <stdint.h>

typedef enum {
    MSR_APIC_BASE   = 0x1B,
    MSR_PAT         = 0x277,
    MSR_EFER        = 0xC0000080,
    MSR_STAR        = 0xC0000081,
    MSR_LSTAR       = 0xC0000082,
    MSR_CSTAR       = 0xC0000083,
    MSR_SFMASK      = 0xC0000084
} msr_t;

/**
 * @brief Read a machine specific register.
 * 
 * @param msr MSR number
 * @return Value in
 */
uint64_t msr_read(uint64_t msr);

/**
 * @brief Write to a machine specific register.
 * 
 * @param msr MSR number
 * @param value Value out
 */
void msr_write(uint64_t msr, uint64_t value);

#endif