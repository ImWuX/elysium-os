#include "string.h"

size_t strlen(const char *str) {
    size_t i = 0;
    while(str[i]) ++i;
    return i;
}

int strcmp(const char *lhs, const char *rhs) {
    size_t i = 0;
    while(lhs[i] == rhs[i])
        if(!lhs[i++]) return 0;
    return lhs[i] < rhs[i] ? -1 : 1;
}

char *strcpy(char *dest, const char *src) {
    size_t i;
    for(i = 0; src[i]; i++) dest[i] = src[i];
    dest[i] = src[i];
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n) {
    size_t i;
    for(i = 0; i < n && src[i]; i++) dest[i] = src[i];
    for(; i < n; i++) dest[i] = 0;
    return dest;
}

void *memset(void *dest, int ch, size_t count) {
    char *temp = (char *) dest;
    for(; count > 0; count--) *temp++ = (unsigned char) ch;
    return dest;
}

void *memcpy(void *dest, const void *src, size_t count) {
    asm volatile("cld");
    asm volatile("rep movsb" : "+D"(dest), "+S"(src), "+c"(count) : : "memory");
    return dest;
}

int memcmp(const void *lhs, const void *rhs, size_t count) {
    for(size_t i = 0; i < count; i++) {
        if(*((unsigned char *) lhs + i) > *((unsigned char *) rhs + i))
            return -1;
        if(*((unsigned char *) lhs + i) < *((unsigned char *) rhs + i))
            return 1;
    }
    return 0;
}