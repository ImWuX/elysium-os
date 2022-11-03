#include "cpuid.h"
#include <cpuid.h>

#define VENDOR_LEAF 0
#define CAPABILITIES_LEAF 1

bool cpuid_check_capability(cpuid_capability_t capability) {
    unsigned int eax, edx, unused;
    __get_cpuid(CAPABILITIES_LEAF, &eax, &unused, &unused, &edx);
    unsigned int reg = eax;
    if(capability >= 32) {
        capability -= 32;
        reg = edx;
    }
    return (reg & (1 << capability)) > 0;
}