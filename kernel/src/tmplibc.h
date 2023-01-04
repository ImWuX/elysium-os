#ifndef TMP_LIBC_H
#define TMP_LIBC_H

#include <stdint.h>
#include <stdbool.h>

#define BIT_SET(src, n) (src |= (1 << n))
#define BIT_UNSET(src, n) (src &= ~(1 << n))

void memcpy(void *source, void *dest, int nbytes);
void memset(uint8_t value, void *dest, int len);
bool memcmp(void *cmp1, void *cmp2, int nbytes);

#endif