#pragma once
#include <stdint.h>
#include <stdarg.h>

typedef void (*kprint_putchar_t)(char ch);

extern kprint_putchar_t g_kprint_putchar;

/**
 * @brief Prints a formatted string. Takes varargs as values
 * @returns count written
 */
int kprintf(const char *fmt, ...);

/**
 * @brief Prints a formatted string. Takes a vararg list as values
 * @returns count written
 */
int kprintv(const char *format, va_list list);