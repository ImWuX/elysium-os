#pragma once
#include <stdint.h>
#include <stddef.h>

#ifdef __ARCH_AMD64
typedef struct stack_frame {
    struct stack_frame *rbp;
    uint64_t rip;
} __attribute__((packed)) stack_frame_t;

extern char *g_panic_symbols;
extern size_t g_panic_symbols_length;

/**
 * @brief Panic with a specific stack frame
 *
 * @param frame Stack frame
 * @param fmt Format string
 */
[[noreturn]] void panic_stack_trace(stack_frame_t *frame, const char *fmt, ...);
#endif

/**
 * @brief Panic
 *
 * @param fmt Format string
*/
[[noreturn]] void panic(const char *fmt, ...);