#include "util.h"

void memcpy(void *src, void *dest, int nbytes) {
    for(int i = 0; i < nbytes; i++)
        *((uint8_t *) dest + i) = *((uint8_t *) src + i);
}

void memset(uint8_t value, void *dest, int len) {
    uint8_t *temp = (uint8_t *) dest;
    for(; len != 0; len--) *temp++ = value;
}