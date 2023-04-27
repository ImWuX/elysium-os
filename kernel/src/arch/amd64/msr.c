#include "msr.h"
#include <cpuid.h>

#define CPUID_CAPABILITY_MSR 5

bool msr_available() {
    unsigned int edx = 0, unused;
    int supported = __get_cpuid(1, &unused, &unused, &unused, &edx);
    return supported && (edx & (1 << CPUID_CAPABILITY_MSR)) > 0;
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