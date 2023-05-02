#include <arch/init.h>
#include <cpuid.h>
#include <panic.h>
#include <memory/hhdm.h>
#include <arch/amd64/gdt.h>
#include <arch/amd64/io.h>
#include <arch/amd64/pic8259.h>
#include <arch/amd64/cpuid.h>

void arch_init(tartarus_parameters_t *boot_params) {
    gdt_initialize();

    if(!cpuid_feature(CPUID_FEAT_MSR)) panic("ARCH/AMD64", "MSRS are not supported on this system");

    uint64_t pat = io_msr_read(IO_MSR_PAT);
    pat &= ~(((uint64_t) 0b111 << 48) | ((uint64_t) 0b111 << 40));
    pat |= ((uint64_t) 0x1 << 48) | ((uint64_t) 0x5 << 40);
    io_msr_write(IO_MSR_PAT, pat);

    pic8259_remap();
    if(!cpuid_feature(CPUID_FEAT_APIC)) {
        panic("ARCH/AMD64", "Legacy PIC is not supported at the moment");
    } else {
        pic8259_disable();
        
    }
}