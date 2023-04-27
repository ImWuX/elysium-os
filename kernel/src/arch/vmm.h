#ifndef ARCH_VMM_H
#define ARCH_VMM_H

#include <memory/vmm.h>

/**
 * @brief Create the initial kernel address space.
 * 
 * @param out Kernel address space
 */
void arch_vmm_create_kernel_address_space(vmm_address_space_t *out);

/**
 * @brief Load a virtual address space.
 * 
 * @param address_space Address space
 */
void arch_vmm_load_address_space(vmm_address_space_t *address_space);

/**
 * @brief Map a virtual address to a physical address.
 *
 * @param address_space Address space
 * @param vaddr Virtual address
 * @param paddr Physical address
 * @param flags Architecture independent flags
 */
void arch_vmm_map(vmm_address_space_t *address_space, uintptr_t vaddr, uintptr_t paddr, uint64_t flags);

/**
 * @brief Calculate highest userspace address for arch.
 * 
 * @returns Highest userspace address
 */
uintptr_t arch_vmm_highest_userspace_addr();

#endif