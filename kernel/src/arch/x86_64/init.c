#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <tartarus.h>
#include <lib/format.h>
#include <common/kprint.h>
#include <memory/hhdm.h>
#include <arch/cpu.h>

uintptr_t g_hhdm_base;
size_t g_hhdm_size;

static void pch(char ch) {
    asm volatile("outb %0, %1" : : "a" (ch), "Nd" (0x3F8));
}

int kprintv(const char *fmt, va_list list) {
	return format(pch, fmt, list);
}

int kprintf(const char *fmt, ...) {
    va_list list;
	va_start(list, fmt);
	int ret = kprintv(fmt, list);
	va_end(list);
	return ret;
}

[[noreturn]] void init(tartarus_boot_info_t *boot_info) {
	g_hhdm_base = boot_info->hhdm_base;
	g_hhdm_size = boot_info->hhdm_size;

    kprintf("HHDM: %#lx (%#lx)\n", g_hhdm_base, g_hhdm_size);

    arch_cpu_halt();
}