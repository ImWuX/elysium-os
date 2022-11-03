#ifndef UTIL_UTIL_H
#define UTIL_UTIL_H

#include <stdint.h>
#include <stdbool.h>

#define FLAG_SET(x, flag) x |= (flag)
#define FLAG_UNSET(x, flag) x &= ~(flag)

void memcpy(void *source, void *dest, int nbytes);
void memset(uint8_t value, void *dest, int len);
bool memcmp(void *cmp1, void *cmp2, int nbytes);

#endif