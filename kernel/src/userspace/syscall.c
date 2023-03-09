#include "syscall.h"
#include "userspace.h"
#include <cpuid.h>
#include <cpu/msr.h>

#define CPUID_CABABILITY_SYSCALL 11

bool syscall_available() {
    unsigned int edx = 0, unused;
    int supported = __get_cpuid(0x80000001, &unused, &unused, &unused, &edx);
    return supported && (edx & (1 << CPUID_CABABILITY_SYSCALL)) > 0;
}

void syscall_initialize() {
    msr_set(MSR_STAR, (msr_get(MSR_STAR) & 0xFFFFFFFF) | ((uint64_t) 0x8 << 32) | (((uint64_t) (0x10 | 3) << 48)));
    msr_set(MSR_LSTAR, syscall_entry);
    msr_set(MSR_SFMASK, msr_get(MSR_SFMASK) | 0xfffffffe);
    msr_set(MSR_EFER, msr_get(MSR_EFER) | 1);
}