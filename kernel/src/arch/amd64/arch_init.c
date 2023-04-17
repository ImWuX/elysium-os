#include <arch/arch_init.h>
#include <arch/amd64/gdt.h>

void arch_init(tartarus_parameters_t *boot_params) {
    gdt_initialize();
}