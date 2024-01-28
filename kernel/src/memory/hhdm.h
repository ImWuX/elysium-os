#pragma once
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Converts a physical address to a virtual HHDM address.
 * @param address physical address
 * @returns virtual address in the HHDM
 */
#define HHDM(ADDRESS) ((uintptr_t) (ADDRESS) + g_hhdm_base)
// TODO: Maybe do some bounds checking here. maybe under a debug flag?

extern uintptr_t g_hhdm_base;
extern size_t g_hhdm_size;