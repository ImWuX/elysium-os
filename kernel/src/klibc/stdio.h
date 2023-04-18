#ifndef KLIBC_STDIO_H
#define KLIBC_STDIO_H

#include <stdint.h>
#include <stdarg.h>

extern int putchar(int character);

typedef unsigned long size_t;

/**
 * @brief Prints a formatted string. Takes varargs as values.
 * 
 * @param fmt Format string
 * @param ... Varargs
 * @return Count of characters written
 */
int printf(const char *fmt, ...);

/**
 * @brief Prints a formatted string. Takes a vararg list as values.
 * 
 * @param fmt Format string
 * @param list Vararg list
 * @return Count of characters written
 */
int vprintf(const char *format, va_list list);

#endif