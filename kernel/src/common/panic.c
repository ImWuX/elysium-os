#include "panic.h"

#include "common/log.h"
#include "arch/cpu.h"

#ifdef __ARCH_X86_64
char *g_panic_symbols;
size_t g_panic_symbols_length;

static void stack_trace(stack_frame_t *stack_frame) {
    if(!g_panic_symbols) return;
    log(LOG_LEVEL_ERROR, "PANIC", "Stack Trace:");
    for(int i = 0; stack_frame && stack_frame->rip && i < 30; i++) {
        uint64_t offset = 0;
        uint64_t address = 0;
        bool skip = false;
        for(uint64_t j = 0; j < g_panic_symbols_length; j++) {
            if(g_panic_symbols[j] == '\n') {
                skip = false;
                address = 0;
                continue;
            }
            if(skip) continue;
            if(g_panic_symbols[j] >= '0' && g_panic_symbols[j] <= '9') {
                address *= 16;
                address += g_panic_symbols[j] - '0';
                continue;
            }
            if(g_panic_symbols[j] >= 'a' && g_panic_symbols[j] <= 'f') {
                address *= 16;
                address += g_panic_symbols[j] - 'a' + 10;
                continue;
            }
            if(g_panic_symbols[j] == ' ' && address >= stack_frame->rip) break;
            skip = true;
            offset = j + 3;
        }

        if(offset >= g_panic_symbols_length) {
            log(LOG_LEVEL_ERROR, "PANIC", "    [UNKNOWN] <%#lx>", stack_frame->rip);
        } else {
            int len = 0;
            while(g_panic_symbols[offset + len] != '\n') len++;
            log(LOG_LEVEL_ERROR, "PANIC", "    %.*s+%lu <%#lx>", len, &g_panic_symbols[offset], address - stack_frame->rip, stack_frame->rip);
        }
        stack_frame = stack_frame->rbp;
    }
}

[[noreturn]] void panic_stack_trace(stack_frame_t *frame, const char *fmt, ...) {
    log(LOG_LEVEL_ERROR, "PANIC", "Kernel Panic");
    va_list list;
	va_start(list, format);
    log_list(LOG_LEVEL_ERROR, "PANIC", fmt, list);
    stack_trace(frame);
	va_end(list);
    arch_cpu_halt();
    __builtin_unreachable();
}
#endif

[[noreturn]] void panic(const char *fmt, ...) {
    log(LOG_LEVEL_ERROR, "PANIC", "Kernel Panic");
    va_list list;
	va_start(list, format);
    log_list(LOG_LEVEL_ERROR, "PANIC", fmt, list);
#ifdef __ARCH_X86_64
    stack_frame_t *stack_frame;
    asm volatile("movq %%rbp, %0" : "=r" (stack_frame));
    stack_trace(stack_frame);
#else
#warning No stack trace implemented for this arch
#endif
	va_end(list);
    arch_cpu_halt();
    __builtin_unreachable();
}