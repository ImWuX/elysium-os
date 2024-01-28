#pragma once
#include <stdint.h>
#include <memory/vmm.h>

#define ARCH_VMM_FLAG_NONE 0
#define ARCH_VMM_FLAG_USER (1 << 0)
#define ARCH_VMM_FLAG_GLOBAL (1 << 1)

/**
 * @brief Initialize the vmm & kernel address space.
 * @returns Kernel address space
 */
vmm_address_space_t *arch_vmm_init();

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
 * @param prot protection
 * @param flags
 */
void arch_vmm_map(vmm_address_space_t *address_space, uintptr_t vaddr, uintptr_t paddr, vmm_prot_t prot, int flags);

/**
 * @brief Unmap a virtual address from address space.
 * @param address_space
 * @param vaddr virtual address
 */
void arch_vmm_unmap(vmm_address_space_t *address_space, uintptr_t vaddr);

/**
 * @brief Translate a virtual address to a physical address.
 * @param address_space
 * @param vaddr virtual address
 * @param out physical address
 * @returns success
 */
bool arch_vmm_physical(vmm_address_space_t *address_space, uintptr_t vaddr, uintptr_t *out);