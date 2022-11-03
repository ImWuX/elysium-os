#include "msr.h"
#include <cpu/cpuid.h>

bool msr_available() {
    return cpuid_check_capability(CPUID_CAPABILITY_EDX_MSR);
}

uint64_t msr_get(uint64_t msr) {
    uint32_t low;
    uint32_t high;
    asm volatile("rdmsr" : "=a" (low), "=d" (high) : "c" (msr));
    return low + ((uint64_t) high << 32);
}

void msr_set(uint64_t msr, uint64_t value) {
    asm volatile("wrmsr" : : "a" ((uint32_t) value), "d" ((uint32_t) (value >> 32)), "c" (msr));
}