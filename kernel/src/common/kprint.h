#pragma once
#include <stdint.h>
#include <stdarg.h>

/**
 * @brief Prints a formatted string
 * @returns char count written
 */
int kprintf(const char *fmt, ...);

/**
 * @brief Prints a formatted string
 * @returns char count written
 */
int kprintv(const char *fmt, va_list list);