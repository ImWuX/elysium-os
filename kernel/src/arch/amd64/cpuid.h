#ifndef ARCH_AMD64_CPUID_H
#define ARCH_AMD64_CPUID_H

#include <stdint.h>

#define CPUID_FEAT_MASK_EXT ((uint64_t) 1 << 32)
#define CPUID_FEAT_MASK_REG_ECX ((uint64_t) 1 << 33)
#define CPUID_FEAT_MASK_REG_EDX ((uint64_t) 1 << 34)

typedef enum {
    CPUID_FEAT_SSE3         = (1 << 0) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_PCLMUL       = (1 << 1) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_DTES64       = (1 << 2) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_MONITOR      = (1 << 3) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_DS_CPL       = (1 << 4) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_VMX          = (1 << 5) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_SMX          = (1 << 6) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_EST          = (1 << 7) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_TM2          = (1 << 8) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_SSSE3        = (1 << 9) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_CID          = (1 << 10) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_SDBG         = (1 << 11) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_FMA          = (1 << 12) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_CX16         = (1 << 13) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_XTPR         = (1 << 14) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_PDCM         = (1 << 15) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_PCID         = (1 << 17) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_DCA          = (1 << 18) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_SSE4_1       = (1 << 19) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_SSE4_2       = (1 << 20) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_X2APIC       = (1 << 21) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_MOVBE        = (1 << 22) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_POPCNT       = (1 << 23) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_TSC_DEADLINE = (1 << 24) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_AES          = (1 << 25) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_XSAVE        = (1 << 26) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_OSXSAVE      = (1 << 27) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_AVX          = (1 << 28) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_F16C         = (1 << 29) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_RDRAND       = (1 << 30) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_HYPERVISOR   = (1 << 31) | CPUID_FEAT_MASK_REG_ECX,
    CPUID_FEAT_FPU          = (1 << 0) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_VME          = (1 << 1) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_DE           = (1 << 2) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_PSE          = (1 << 3) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_TSC          = (1 << 4) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_MSR          = (1 << 5) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_PAE          = (1 << 6) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_MCE          = (1 << 7) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_CX8          = (1 << 8) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_APIC         = (1 << 9) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_SEP          = (1 << 11) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_MTRR         = (1 << 12) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_PGE          = (1 << 13) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_MCA          = (1 << 14) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_CMOV         = (1 << 15) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_PAT          = (1 << 16) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_PSE36        = (1 << 17) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_PSN          = (1 << 18) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_CLFLUSH      = (1 << 19) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_DS           = (1 << 21) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_ACPI         = (1 << 22) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_MMX          = (1 << 23) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_FXSR         = (1 << 24) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_SSE          = (1 << 25) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_SSE2         = (1 << 26) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_SS           = (1 << 27) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_HTT          = (1 << 28) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_TM           = (1 << 29) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_IA64         = (1 << 30) | CPUID_FEAT_MASK_REG_EDX,
    CPUID_FEAT_PBE          = (1 << 31) | CPUID_FEAT_MASK_REG_EDX
} cpuid_features_t;

/**
 * @brief Test for a feature exposed in CPUID
 *
 * @param feature A feature in CPUID
 * @retval true Supported
 * @retval false Unsupported
 */
bool cpuid_feature(cpuid_features_t feature);

#endif