#ifndef MEMORY_HHDM_H
#define MEMORY_HHDM_H

#include <stdint.h>

/**
 * @brief Convert a physical address to a HHDM address
 *
 * @param address Physical address
 * @return HHDM virtual address
 */
#define HHDM(address) ((uintptr_t) (address) + g_hhdm_address)

extern uintptr_t g_hhdm_address;

#endif