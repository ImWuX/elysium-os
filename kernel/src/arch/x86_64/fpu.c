#include "fpu.h"
#include <common/assert.h>
#include <arch/x86_64/cpuid.h>

uint32_t g_x86_64_fpu_area_size = 0;
void (* g_x86_64_fpu_save)(void *area) = 0;
void (* g_x86_64_fpu_restore)(void *area) = 0;

static inline void xsave(void *area) {
    asm volatile ("xsave (%0)" : : "r" (area), "a"(0xffffffff), "d"(0xffffffff) : "memory");
}

static inline void xrstor(void *area) {
    asm volatile ("xrstor (%0)" : : "r" (area), "a"(0xffffffff), "d"(0xffffffff) : "memory");
}

static inline void fxsave(void *area) {
    asm volatile ("fxsave (%0)" : : "r" (area) : "memory");
}

static inline void fxrstor(void *area) {
    asm volatile ("fxrstor (%0)" : : "r" (area) : "memory");
}

void x86_64_fpu_init() {
    if(x86_64_cpuid_feature(X86_64_CPUID_FEATURE_XSAVE)) {
        uint32_t area_size;
        ASSERT(!x86_64_cpuid_register(0xD, X86_64_CPUID_REGISTER_ECX, &area_size));
        g_x86_64_fpu_area_size = area_size;
        g_x86_64_fpu_save = xsave;
        g_x86_64_fpu_restore = xrstor;
    } else {
        g_x86_64_fpu_area_size = 512;
        g_x86_64_fpu_save = fxsave;
        g_x86_64_fpu_restore = fxrstor;
    }
}

void x86_64_fpu_init_cpu() {
    /* Enable FPU */
    uint64_t cr0;
    asm volatile("mov %%cr0, %0" : "=r" (cr0) : : "memory");
    cr0 &= ~(1 << 2); /* CR0.EM */
    cr0 |= 1 << 1; /* CR0.MP */
    asm volatile("mov %0, %%cr0" : : "r" (cr0) : "memory");

    /* Enable MMX & friends */
    uint64_t cr4;
    asm volatile("mov %%cr4, %0" : "=r" (cr4) : : "memory");
    cr4 |= 1 << 9; /* CR4.OSFXSR */
    cr4 |= 1 << 10; /* CR4.OSXMMEXCPT */
    asm volatile("mov %0, %%cr4" : : "r" (cr4) : "memory");

    if(x86_64_cpuid_feature(X86_64_CPUID_FEATURE_XSAVE)) {
        asm volatile("mov %%cr4, %0" : "=r" (cr4) : : "memory");
        cr4 |= 1 << 18; /* CR4.OSXSAVE */
        asm volatile("mov %0, %%cr4" : : "r" (cr4) : "memory");

        uint64_t xcr0 = 0;
        xcr0 |= 1 << 0; /* XCR0.X87 */
        xcr0 |= 1 << 1; /* XCR0.SSE */
        if(x86_64_cpuid_feature(X86_64_CPUID_FEATURE_AVX)) xcr0 |= 1 << 2; /* XCR0.AVX */
        if(x86_64_cpuid_feature(X86_64_CPUID_FEATURE_AVX512)) {
            xcr0 |= 1 << 5; /* XCR0.opmask */
            xcr0 |= 1 << 6; /* XCR0.ZMM_Hi256 */
            xcr0 |= 1 << 7; /* XCR0.Hi16_ZMM */
        }
        asm volatile("xsetbv" : : "a" (xcr0), "d" (xcr0 >> 32), "c" (0) : "memory");
    }
}
