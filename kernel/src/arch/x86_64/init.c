#include <stdarg.h>
#include <tartarus.h>
#include <lib/format.h>
#include <common/kprint.h>
#include <arch/cpu.h>

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
    kprintf("Hello World!\n");

    arch_cpu_halt();
}