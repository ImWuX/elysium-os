#include "util.h"

void memcpy(void *src, void *dest, int nbytes) {
    for(int i = 0; i < nbytes; i++)
        *((uint8_t *) dest + i) = *((uint8_t *) src + i);
}

void memset(uint8_t value, void *dest, int len) {
    uint8_t *temp = (uint8_t *) dest;
    for(; len > 0; len--) *temp++ = value;
}

bool memcmp(void *cmp1, void *cmp2, int nbytes) {
    for(int i = 0; i < nbytes; i++) {
        if(*((uint8_t *) cmp1 + i) != *((uint8_t *) cmp2 + i))
            return false;
    }
    return true;
}