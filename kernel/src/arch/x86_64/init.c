#include <tartarus.h>

static void pch(char ch) {
    asm volatile("outb %0, %1" : : "a" (ch), "Nd" (0x3F8));
}

static void pstr(char *str) {
    while(*str != 0) pch(*str++);
}

[[noreturn]] void init(tartarus_boot_info_t *boot_info) {
    pstr("Hello World!\n");

    for(;;) asm volatile("hlt");
    __builtin_unreachable();
}