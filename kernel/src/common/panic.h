#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __ARCH_X86_64
typedef struct stack_frame {
    struct stack_frame *rbp;
    uint64_t rip;
} __attribute__((packed)) stack_frame_t;

extern char *g_panic_symbols;
extern size_t g_panic_symbols_length;

/**
 * @brief Log error and stack frame, then halth.
 * @param frame stack frame
 */
[[noreturn]] void panic_stack_trace(stack_frame_t *frame, const char *fmt, ...);
#endif

/**
 * @brief Log error, then halth.
*/
[[noreturn]] void panic(const char *fmt, ...);