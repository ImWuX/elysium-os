#include <tartarus.h>
#include <memory/hhdm.h>
#include <lib/kprint.h>
#include <lib/panic.h>
#include <lib/string.h>
#include <arch/cpu.h>
#include <arch/x86_64/port.h>

uintptr_t g_hhdm_base;

static void pch(char ch) {
    port_outb(0x3F8, ch);
}

[[noreturn]] void init(tartarus_boot_info_t *boot_info) {
    g_hhdm_base = boot_info->hhdm_base;

    g_kprint_putchar = pch;

    kprintf("Elysium pre-alpha\n");

    for(uint16_t i = 0; i < boot_info->module_count; i++) {
        tartarus_module_t *module = &boot_info->modules[i];
        if(memcmp(module->name, "KERNSYMBTXT", 11) != 0) continue;
        g_panic_symbols = (char *) HHDM(module->paddr);
        g_panic_symbols_length = module->size;
    }

    panic("test panic\n");

    arch_cpu_halt();
    __builtin_unreachable();
}