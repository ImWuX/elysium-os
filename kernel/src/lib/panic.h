#pragma once

/**
 * @brief Log an issue, log stackframe, halt the core
 *
 * @param fmt Format string
*/
[[noreturn]] void panic(const char *fmt, ...);

/**
 * @brief Log an issue, halt the core
 *
 * @param fmt Format string
*/
[[noreturn]] void panic_no_stack_frame(const char *fmt, ...);