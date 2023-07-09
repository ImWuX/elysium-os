#ifndef MEMORY_VMM_H
#define MEMORY_VMM_H

#include <stdint.h>
#include <stddef.h>
#include <arch/types.h>
#include <memory/pmm.h>

#define EOCCUPIED 4000

#define VMM_DEFAULT_KERNEL_FLAGS VMM_FLAGS_WRITE

typedef enum {
    VMM_FLAGS_WRITE = (1 << 0),
    VMM_FLAGS_EXEC = (1 << 1),
    VMM_FLAGS_USER = (1 << 2)
} vmm_flags_t;

typedef struct vmm_anon {
    struct vmm_anon *next;
    uintptr_t offset;
    pmm_page_t *page;
} vmm_anon_t;

typedef struct {
    
} vmm_object_t;

typedef struct vmm_range {
    uintptr_t start;
    uintptr_t end;
    uint64_t flags;
    bool is_anon;
    bool is_direct;
    union {
        vmm_anon_t *anon;
        uintptr_t paddr;
    };
    struct vmm_range *next;
} vmm_range_t;

typedef struct {
    vmm_range_t *ranges;
    arch_vmm_address_space_t archdep;
} vmm_address_space_t;

/**
 * @brief Allocate a number of wired pages.
 *
 * @param address_space Address space
 * @param vaddr Virtual address for pages
 * @param npages Number of pages
 * @param flags Flags
 * @return 0 on success, negative errno on failure
 */
int vmm_alloc_wired(vmm_address_space_t *address_space, uintptr_t vaddr, size_t npages, uint64_t flags);

/**
 * @brief Map a range of virtual memory to a specific physical address.
 *
 * @param address_space Address space
 * @param vaddr Virtual address of range
 * @param paddr Physical address to map into
 * @param size Size of range
 * @return 0 on success, negative errno on failure
 */
int vmm_map_direct(vmm_address_space_t *address_space, uintptr_t vaddr, uintptr_t paddr, size_t size);

/**
 * @brief Allocate a range of anonymous memory in a particular address space.
 *
 * @param address_space Address space
 * @param vaddr Virtual address of range
 * @param size Size of range
 * @return 0 on success, negative errno on failure
 */
int vmm_alloc(vmm_address_space_t *address_space, uintptr_t vaddr, size_t size);

/**
 * @brief Free a range of memory in a particular address space.
 *
 * @param address_space Address space
 * @param vaddr Virtual address of range
 * @param size Size of range
 * @return 0 on success, negative errno on failure
 */
int vmm_free(vmm_address_space_t *address_space, uintptr_t vaddr, size_t size);

#endif