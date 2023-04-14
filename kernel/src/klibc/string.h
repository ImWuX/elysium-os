#ifndef KLIBC_STRING_H
#define KLIBC_STRING_H

#include <stdint.h>
#include <stddef.h>

size_t strlen(const char *str);
int strcmp(const char *lhs, const char *rhs);
char *strcpy(char *dest, const char *src);

void *memset(void *dest, int ch, size_t count);
void *memcpy(void *dest, const void *src, size_t count);
int memcmp(const void *lhs, const void *rhs, size_t count);

#endif