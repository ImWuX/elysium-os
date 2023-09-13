#pragma once
#include <memory/vmm.h>

/**
 * @brief Initialize the VMM
 */
void arch_vmm_init();

/**
 * @brief Load a virtual address space
 *
 * @param address_space Address space
 */
void arch_vmm_load_address_space(vmm_address_space_t *address_space);

/**
 * @brief Map a virtual address to a physical address
 *
 * @param address_space Address space
 * @param vaddr Virtual address
 * @param paddr Physical address
 * @param flags Architecture independent flags
 */
void arch_vmm_map(vmm_address_space_t *address_space, uintptr_t vaddr, uintptr_t paddr, uint64_t flags);

/**
 * @brief Retrieve the physical address from an address space
 *
 * @param address_space Address space
 * @param virtual_address Address space
 * @return Physical address
 */
uintptr_t arch_vmm_physical(vmm_address_space_t *address_space, uintptr_t vaddr);

/**
 * @brief Calculate highest userspace address for arch
 *
 * @returns Highest userspace address
 */
uintptr_t arch_vmm_highest_userspace_addr();