#ifndef LIBC_STDIO_H
#define LIBC_STDIO_H

#include <stdint.h>
#include <stdarg.h>

extern int putchar(int character);

typedef unsigned long size_t;

int printf(const char *fmt, ...);
int vprintf(const char *format, va_list list);

#endif