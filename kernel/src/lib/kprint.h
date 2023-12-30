#pragma once
#include <stdint.h>
#include <stdarg.h>

typedef void (*kprint_putchar_t)(char ch);

extern kprint_putchar_t g_kprint_putchar;

/**
 * @brief Prints a formatted string. Takes varargs as values
 *
 * @param fmt Format string
 * @param ... Varargs
 * @return Count of characters written
 */
int kprintf(const char *fmt, ...);

/**
 * @brief Prints a formatted string. Takes a vararg list as values
 *
 * @param fmt Format string
 * @param list Vararg list
 * @return Count of characters written
 */
int kprintv(const char *format, va_list list);