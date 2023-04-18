#ifndef ARCH_TYPES_H
#define ARCH_TYPES_H

#ifdef __ARCH_AMD64
#include "amd64/types.h"
#else
#error "This arch has not implemented types.h"
#endif

#endif