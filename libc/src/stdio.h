#ifndef LIBC_STDIO_H
#define LIBC_STDIO_H

#include <stdint.h>

extern int putchar(int character);

typedef unsigned long size_t;

void printf(const char *fmt, ...);
void printe(const char *msg);

#endif