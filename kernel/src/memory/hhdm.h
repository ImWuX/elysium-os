#ifndef MEMORY_HHDM_H
#define MEMORY_HHFM_H

#include <stdint.h>

#define HHDM(address) ((uint64_t) address + g_hhdm_address)

extern uint64_t g_hhdm_address;

#endif