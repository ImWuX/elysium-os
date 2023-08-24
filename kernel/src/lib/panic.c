#include "panic.h"
#include <lib/kprint.h>

#ifdef __ARCH_AMD64
    typedef struct stack_frame {
        struct stack_frame *rbp;
        uint64_t rip;
    } __attribute__((packed)) stack_frame_t;
#endif

[[noreturn]] static void halt() {
    #ifdef __ARCH_AMD64
        asm volatile("cli");
        for(;;) asm volatile("hlt");
    #else
        for(;;);
    #endif
    __builtin_unreachable();
}

[[noreturn]] void panic(const char *fmt, ...) {
    kprintf("KERNEL PANIC\n");
    va_list list;
	va_start(list, format);
	kprintv(fmt, list);
	va_end(list);
    // TODO: Stack frame
    halt();
}

[[noreturn]] void panic_no_stack_frame(const char *fmt, ...) {
    kprintf("KERNEL PANIC\n");
    va_list list;
	va_start(list, format);
	kprintv(fmt, list);
	va_end(list);
    halt();
}