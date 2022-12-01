#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

void memcpy(void *source, void *dest, int nbytes);
void memset(uint8_t value, void *dest, int len);

#endif