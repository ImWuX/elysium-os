#ifndef MEMORY_HHDM_H
#define MEMORY_HHDM_H

#include <stdint.h>

#define HHDM(address) ((uintptr_t) (address) + g_hhdm_address)

extern uintptr_t g_hhdm_address;

#endif