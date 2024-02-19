#include "mem.h"

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