#include "cpuid.h"
#include <cpuid.h>

#define CPUID_EXT 0x80000000
#define FEATURE_MASK 0xFFFFFFFF

bool cpuid_feature(cpuid_features_t feature) {
    uint32_t eax = 0, ebx = 0, ecx = 0, edx = 0;
    if(__get_cpuid((feature & CPUID_FEAT_MASK_EXT) ? CPUID_EXT + 1 : 1, &eax, &ebx, &ecx, &edx) == 0) return false;
    if(feature & CPUID_FEAT_MASK_REG_ECX) return ecx & (feature & FEATURE_MASK);
    if(feature & CPUID_FEAT_MASK_REG_EDX) return edx & (feature & FEATURE_MASK);
    return false;
}