#pragma once
#include <stdint.h>

typedef enum {
    MSR_APIC_BASE       = 0x1B,
    MSR_PAT             = 0x277,
    MSR_EFER            = 0xC0000080,
    MSR_STAR            = 0xC0000081,
    MSR_LSTAR           = 0xC0000082,
    MSR_CSTAR           = 0xC0000083,
    MSR_SFMASK          = 0xC0000084,
    MSR_FS_BASE         = 0xC0000100,
    MSR_GS_BASE         = 0xC0000101,
    MSR_KERNEL_GS_BASE  = 0xC0000102
} msr_t;

/**
 * @brief Read from machine specific register.
 * @param msr
 * @returns value
 */
static inline uint64_t msr_read(uint64_t msr) {
    uint32_t low;
    uint32_t high;
    asm volatile("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));
    return low + ((uint64_t) high << 32);
}

/**
 * @brief Write to machine specific register.
 * @param msr
 * @param value
 */
static inline void msr_write(uint64_t msr, uint64_t value) {
    asm volatile("wrmsr" : : "a" ((uint32_t) value), "d" ((uint32_t) (value >> 32)), "c" (msr));
}