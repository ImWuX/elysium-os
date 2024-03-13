#pragma once
#include <stdint.h>
#include <stddef.h>

/**
 * @brief Converts a physical address to a virtual HHDM address
 * @param ADDRESS physical address
 * @returns virtual address inside the HHDM
 */
#define HHDM(ADDRESS) ((uintptr_t) (ADDRESS) + g_hhdm_base)

/**
 * @brief Converts a virtual HHDM address to a physical address
 * @param ADDRESS virtual address inside the HHDM
 * @returns physical address
 */
#define HHDM_TO_PHYS(ADDRESS) ((uintptr_t) (ADDRESS) - g_hhdm_base)

extern uintptr_t g_hhdm_base;
extern size_t g_hhdm_size; // TODO: Maybe do some bounds checking here. maybe under a debug flag?