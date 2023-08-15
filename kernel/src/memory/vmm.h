#ifndef MEMORY_VMM_H
#define MEMORY_VMM_H

#include <stdint.h>
#include <stddef.h>
#include <arch/types.h>
#include <memory/pmm.h>
#include <lib/slock.h>
#include <lib/list.h>

#define VMM_DEFAULT_KERNEL_FLAGS VMM_FLAGS_WRITE

typedef enum {
    VMM_FLAGS_WRITE = (1 << 0),
    VMM_FLAGS_EXEC = (1 << 1),
    VMM_FLAGS_USER = (1 << 2)
} vmm_flags_t;

typedef struct {
    struct vmm_address_space *address_space;
    uintptr_t base;
    size_t length;
    list_t list;
} vmm_segment_t;

typedef struct vmm_address_space {
    slock_t lock;
    list_t segments;
    arch_vmm_address_space_t archdep;
} vmm_address_space_t;

extern vmm_address_space_t g_kernel_address_space;

/**
 * @brief Allocate a number of wired pages
 *
 * @param as Address space
 * @param vaddr Virtual address for pages
 * @param npages Number of pages
 * @param flags Flags
 * @return 0 on success, negative errno on failure
 */
int vmm_alloc_wired(vmm_address_space_t *as, uintptr_t vaddr, size_t npages, uint64_t flags);

/**
 * @brief 
 *
 * @param as Address space
 * @param vaddr Virtual address to map into
 * @param npages Number of pages
 * @param paddr Physical address to map
 * @param flags Flags
 * @return 0 on success, negative errno on failure
 */
int vmm_alloc_direct(vmm_address_space_t *as, uintptr_t vaddr, size_t npages, uintptr_t paddr, uint64_t flags);

#endif