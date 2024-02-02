#include "cpu.h"
#include <stddef.h>
#include <arch/cpu.h>

void arch_cpu_relax() {
    __builtin_ia32_pause();
}

[[noreturn]] void arch_cpu_halt() {
    asm volatile("cli");
    for(;;) asm volatile("hlt");
    __builtin_unreachable();
}

cpu_t *arch_cpu_current() {
    x86_64_cpu_t *cpu = NULL;
    asm volatile("mov %%gs:0, %0" : "=r" (cpu));
    return &cpu->common;
}