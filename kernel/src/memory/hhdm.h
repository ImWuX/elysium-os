#pragma once
#include <stdint.h>

/**
 * @brief Converts a physical address to a virtual HHDM address.
 * @param address physical address
 * @return virtual address in the HHDM
 */
#define HHDM(ADDRESS) ((uintptr_t) (ADDRESS) + g_hhdm_base)

extern uintptr_t g_hhdm_base;