#include "cpuid.h"
#include <cpuid.h>

bool cpuid_feature(cpuid_feature_t feature) {
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
    if(__get_cpuid(feature.leaf, &eax, &ebx, &ecx, &edx) == 0) return false;
    switch(feature.reg) {
        case CPUID_REGISTER_EAX: return (eax & (1 << feature.bit));
        case CPUID_REGISTER_EBX: return (ebx & (1 << feature.bit));
        case CPUID_REGISTER_ECX: return (ecx & (1 << feature.bit));
        case CPUID_REGISTER_EDX: return (edx & (1 << feature.bit));
    }
    return false;
}

bool cpuid_register(uint32_t leaf, cpuid_register_t reg, uint32_t *out) {
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
    if(__get_cpuid(leaf, &eax, &ebx, &ecx, &edx) == 0) return true;
    switch(reg) {
        case CPUID_REGISTER_EAX: *out = eax; break;
        case CPUID_REGISTER_EBX: *out = ebx; break;
        case CPUID_REGISTER_ECX: *out = ecx; break;
        case CPUID_REGISTER_EDX: *out = edx; break;
    }
    return false;
}