#include "panic.h"
#include <lib/kprint.h>

[[noreturn]] void panic(char *location, char *msg) {
    kprintf("KERNEL PANIC\n[%s] %s\n", location, msg);
    asm volatile("cli");
    while(true) asm volatile("hlt");
    __builtin_unreachable();
}