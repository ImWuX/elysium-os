#ifndef LIBC_STRING_H
#define LIBC_STRING_H

#include <stdint.h>
#include <stdio.h>

size_t strlen(const char *str);
int strcmp(const char *lhs, const char *rhs);

#endif