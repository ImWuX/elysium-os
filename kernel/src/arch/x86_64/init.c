#include <tartarus.h>
#include <lib/kprint.h>
#include <arch/x86_64/port.h>

static void pch(char ch) {
    port_outb(0x3F8, ch);
}

[[noreturn]] void init(tartarus_boot_info_t *boot_info) {
    g_kprint_putchar = pch;

    kprintf("Hello World\n");

    for(;;);
    __builtin_unreachable();
}