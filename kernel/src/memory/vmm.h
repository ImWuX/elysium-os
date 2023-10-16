#pragma once
#include <stdint.h>
#include <stddef.h>
#include <arch/types.h>
#include <memory/pmm.h>
#include <lib/slock.h>
#include <lib/list.h>

typedef enum {
    VMM_PROT_WRITE = (1 << 0),
    VMM_PROT_EXEC = (1 << 1),
    VMM_PROT_USER = (1 << 2)
} vmm_prot_t;

typedef struct {
    struct vmm_address_space *address_space;
    uintptr_t base;
    size_t length;
    int protection;
    struct vmm_segment_ops *ops;
    void *data;
    list_t list;
} vmm_segment_t;

typedef struct vmm_segment_ops {
    int (* map)(vmm_segment_t *segment, uintptr_t base, size_t length);
    int (* unmap)(vmm_segment_t *segment, uintptr_t base, size_t length);
    bool (* fault)(vmm_segment_t *segment, uintptr_t address);
    void (* free)(vmm_segment_t *segment);
} vmm_segment_ops_t;

typedef struct vmm_address_space {
    slock_t lock;
    list_t segments;
} vmm_address_space_t;

extern vmm_address_space_t *g_kernel_address_space;

/**
 * @brief Maps a segment to its address space
 * @warning Frees the segment if the operation failed
 *
 * @param segment Segment
 * @return negative errno on failure, zero on success
 */
int vmm_map(vmm_segment_t *segment);

/**
 * @brief Unmaps a region
 *
 * @param as Address space
 * @param vaddr Base
 * @param length Length
 * @return negative errno on failure, zero on success
 */
int vmm_unmap(vmm_address_space_t *as, uintptr_t vaddr, size_t length);

/**
 * @brief Maps a region as anonymous memory
 *
 * @param as Address space
 * @param vaddr Base
 * @param length Length
 * @param prot Protection flags
 * @param wired Is memory wired
 * @return negative errno on failure, zero on success
 */
int vmm_map_anon(vmm_address_space_t *as, uintptr_t vaddr, size_t length, int prot, bool wired);

/**
 * @brief Maps a region as a direct map
 *
 * @param as Address space
 * @param vaddr Base
 * @param length Length
 * @param prot Protection flags
 * @param paddr Physical address to map
 * @return negative errno on failure, zero on success
 */
int vmm_map_direct(vmm_address_space_t *as, uintptr_t vaddr, size_t length, int prot, uintptr_t paddr);

/**
 * @brief Handle a pagefault
 *
 * @param as Address space
 * @param address Fault address
 * @return true if fault was a handled
 */
bool vmm_fault(vmm_address_space_t *as, uintptr_t address);