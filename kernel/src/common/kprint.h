#pragma once
#include <stdint.h>
#include <stdarg.h>

typedef void (*kprint_putchar_t)(char ch);

extern kprint_putchar_t g_kprint_putchar;

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