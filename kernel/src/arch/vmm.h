#pragma once
#include <stdint.h>
#include <memory/vmm.h>

typedef enum {
    ARCH_VMM_FLAG_WRITE = (1 << 0),
    ARCH_VMM_FLAG_EXEC = (1 << 1),
    ARCH_VMM_FLAG_USER = (1 << 2),
    ARCH_VMM_FLAG_GLOBAL = (1 << 3)
} arch_vmm_flags_t;

/**
 * @brief Initialize the vmm & kernel address space.
 */
void arch_vmm_init();

/**
 * @brief Load a virtual address space.
 * @param address_space
 */
void arch_vmm_load_address_space(vmm_address_space_t *address_space);

/**
 * @brief Map a virtual address to a physical address.
 * @param address_space
 * @param vaddr virtual address
 * @param paddr physical address
 * @param flags
 */
void arch_vmm_map(vmm_address_space_t *address_space, uintptr_t vaddr, uintptr_t paddr, int flags);
