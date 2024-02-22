#pragma once
#include <stdint.h>
#include <memory/vmm.h>

#define ARCH_VMM_FLAG_NONE 0
#define ARCH_VMM_FLAG_USER (1 << 0)
#define ARCH_VMM_FLAG_GLOBAL (1 << 1)

/**
 * @brief Load a virtual address space
 */
void arch_vmm_load_address_space(vmm_address_space_t *address_space);

/**
 * @brief Map a virtual address to a physical address
 */
void arch_vmm_map(vmm_address_space_t *address_space, uintptr_t vaddr, uintptr_t paddr, vmm_protection_t prot, int flags);

/**
 * @brief Unmap a virtual address from address space
 */
void arch_vmm_unmap(vmm_address_space_t *address_space, uintptr_t vaddr);

/**
 * @brief Translate a virtual address to a physical address
 * @param out physical address
 * @returns true = success
 */
bool arch_vmm_physical(vmm_address_space_t *address_space, uintptr_t vaddr, uintptr_t *out);