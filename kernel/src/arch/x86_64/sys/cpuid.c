#include "cpuid.h"
#include <cpuid.h>

bool x86_64_cpuid_feature(x86_64_cpuid_feature_t feature) {
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
    if(__get_cpuid(feature.leaf, &eax, &ebx, &ecx, &edx) == 0) return false;
    switch(feature.reg) {
        case X86_64_CPUID_REGISTER_EAX: return (eax & (1 << feature.bit));
        case X86_64_CPUID_REGISTER_EBX: return (ebx & (1 << feature.bit));
        case X86_64_CPUID_REGISTER_ECX: return (ecx & (1 << feature.bit));
        case X86_64_CPUID_REGISTER_EDX: return (edx & (1 << feature.bit));
    }
    return false;
}

bool x86_64_cpuid_register(uint32_t leaf, x86_64_cpuid_register_t reg, uint32_t *out) {
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
    if(__get_cpuid(leaf, &eax, &ebx, &ecx, &edx) == 0) return true;
    switch(reg) {
        case X86_64_CPUID_REGISTER_EAX: *out = eax; break;
        case X86_64_CPUID_REGISTER_EBX: *out = ebx; break;
        case X86_64_CPUID_REGISTER_ECX: *out = ecx; break;
        case X86_64_CPUID_REGISTER_EDX: *out = edx; break;
    }
    return false;
}