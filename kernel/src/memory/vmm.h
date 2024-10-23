#pragma once
#include <stdint.h>
#include <stddef.h>
#include <lib/list.h>
#include <common/spinlock.h>

#define VMM_PROT_NONE 0
#define VMM_PROT_READ (1 << 1)
#define VMM_PROT_WRITE (1 << 2)
#define VMM_PROT_EXEC (1 << 3)

#define VMM_FLAG_NONE 0
#define VMM_FLAG_FIXED (1 << 1)
#define VMM_FLAG_NO_DEMAND (1 << 2)

#define VMM_FLAG_ANON_ZERO (1 << 10)

#define VMM_FAULT_NONPRESENT (1 << 0)

typedef uint64_t vmm_flags_t;
typedef uint8_t vmm_protection_t;

typedef enum {
    VMM_CACHE_STANDARD,
    VMM_CACHE_WRITE_COMBINE
} vmm_cache_t;

typedef enum {
    VMM_SEGMENT_TYPE_ANON,
    VMM_SEGMENT_TYPE_DIRECT
} vmm_segment_type_t;

typedef struct {
    spinlock_t lock;
    list_t segments;
    uintptr_t start, end;
} vmm_address_space_t;

typedef union {
    struct { bool back_zeroed; } anon;
    struct { uintptr_t physical_address; } direct;
} vmm_segment_type_specific_data_t;

typedef struct vmm_segment {
    vmm_address_space_t *address_space;
    uintptr_t base, length;
    vmm_segment_type_t type;
    vmm_protection_t protection;
    vmm_cache_t cache;
    list_element_t list_elem;
    vmm_segment_type_specific_data_t type_specific_data;
} vmm_segment_t;

extern vmm_address_space_t *g_vmm_kernel_address_space;

/**
 * @brief Map a region of anonymous memory
 * @param address_space
 * @param hint page aligned address
 * @param length page aligned length
 * @param prot protection
 * @param flags
 */
void *vmm_map_anon(vmm_address_space_t *address_space, void *hint, size_t length, vmm_protection_t prot, vmm_cache_t cache, vmm_flags_t flags);

/**
 * @brief Map a region of direct memory
 * @param address_space
 * @param hint page aligned address
 * @param length page aligned length
 * @param prot protection
 * @param flags
 * @param physical_address
 */
void *vmm_map_direct(vmm_address_space_t *address_space, void *hint, size_t length, vmm_protection_t prot, vmm_cache_t cache, vmm_flags_t flags, uintptr_t physical_address);

/**
 * @brief Unmap a region of memory
 * @param address_space
 * @param address page aligned address
 * @param length page aligned length
 */
void vmm_unmap(vmm_address_space_t *address_space, void *address, size_t length);

/**
 * @brief Handle a virtual memory fault
 * @param address_space
 * @param address
 * @param flags fault flags
 * @returns fault handled
 */
bool vmm_fault(vmm_address_space_t *address_space, uintptr_t address, int flags);

/**
 * @brief Copies data to another address space
 */
size_t vmm_copy_to(vmm_address_space_t *dest_as, uintptr_t dest_addr, void *src, size_t count);

/**
 * @brief Copies data from another address space
 */
size_t vmm_copy_from(void *dest, vmm_address_space_t *src_as, uintptr_t src_addr, size_t count);