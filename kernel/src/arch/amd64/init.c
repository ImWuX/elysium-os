#include <arch/init.h>
#include <panic.h>
#include <arch/amd64/gdt.h>
#include <arch/amd64/msr.h>
#include <memory/hhdm.h>

void arch_init(tartarus_parameters_t *boot_params) {
    gdt_initialize();

    if(!msr_available()) panic("ARCH/AMD64", "MSRS are not supported on this system");

    uint64_t pat = msr_get(MSR_PAT);
    pat &= ~(((uint64_t) 0b111 << 48) | ((uint64_t) 0b111 << 40));
    pat |= ((uint64_t) 0x1 << 48) | ((uint64_t) 0x5 << 40);
    msr_set(MSR_PAT, pat);
}