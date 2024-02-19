#include <tartarus.h>
#include <arch/cpu.h>

static void pch(char ch) {
    asm volatile("outb %0, %1" : : "a" (ch), "Nd" (0x3F8));
}

[[noreturn]] void init(tartarus_boot_info_t *boot_info) {

    arch_cpu_halt();
}