#pragma once
#include <stdint.h>
#include <stddef.h>
#include <lib/list.h>
#include <lib/spinlock.h>

#define VMM_PROT_NONE 0
#define VMM_PROT_READ (1 << 1)
#define VMM_PROT_WRITE (1 << 2)
#define VMM_PROT_EXEC (1 << 3)

#define VMM_FLAG_NONE 0
#define VMM_FLAG_FIXED (1 << 1)

#define VMM_FAULT_NONPRESENT (1 << 0)

typedef uint64_t vmm_flags_t;
typedef uint8_t vmm_prot_t;

typedef struct {
    spinlock_t lock;
    list_t segments;
    uintptr_t start;
    uintptr_t end;
} vmm_address_space_t;

struct vmm_segment;
typedef struct seg_driver {
    char *name;
    struct {
        void (* attach)(struct vmm_segment *segment);
        void (* detach)(struct vmm_segment *segment);
        bool (* fault)(struct vmm_segment *segment, uintptr_t address, int flags);
    } ops;
} seg_driver_t;

typedef struct vmm_segment {
    vmm_address_space_t *address_space;
    uintptr_t base;
    uintptr_t length;
    vmm_prot_t protection;
    list_element_t list_elem;
    seg_driver_t *driver;
    void *driver_data;
} vmm_segment_t;

extern vmm_address_space_t *g_vmm_kernel_address_space;

extern seg_driver_t g_seg_anon;

/**
 * @brief Map a region of memory.
 * @param address_space
 * @param address page aligned address
 * @param length page aligned length
 * @param prot protection
 * @param flags
 * @param driver segment driver
 * @param driver_data private segment driver data
 * @returns mapped region
 */
void *vmm_map(vmm_address_space_t *address_space, void *address, size_t length, vmm_prot_t prot, vmm_flags_t flags, seg_driver_t *driver, void *driver_data);

/**
 * @brief Unmap a region of memory.
 * @param address_space
 * @param address page aligned address
 * @param length page aligned length
 */
void vmm_unmap(vmm_address_space_t *address_space, void *address, size_t length);

/**
 * @brief Handle a virtual memory fault.
 * @param address_space
 * @param address
 * @param flags fault flagsS
 * @returns fault handled
 */
bool vmm_fault(vmm_address_space_t *address_space, uintptr_t address, int flags);