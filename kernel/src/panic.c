#include "panic.h"
#include <stdbool.h>

noreturn void panic(char *location, char *msg) {
    asm volatile("cli\nhlt");
    __builtin_unreachable();
}