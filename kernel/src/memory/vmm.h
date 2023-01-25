#ifndef MEMORY_VMM_H
#define MEMORY_VMM_H

#include <stdint.h>

typedef struct {
    uint64_t entries[512];
} __attribute__((packed)) vmm_page_table_t;

typedef enum {
    VMM_PT_FLAG_PRESENT = 1,
    VMM_PT_FLAG_READWRITE = (1 << 1),
    VMM_PT_FLAG_SUPERVISOR = (1 << 2),
    VMM_PT_FLAG_WRITETHROUGH = (1 << 3),
    VMM_PT_FLAG_DISABLECACHE = (1 << 4),
    VMM_PT_FLAG_ACCESSED = (1 << 5),
} vmm_pt_flags_t;

void vmm_initialize(uint64_t pml4_address);
uint64_t vmm_physical(void *virtual_address);
void vmm_mapf(void *physical_address, void *virtual_address, uint64_t flags);
void vmm_map(void *physical_address, void *virtual_address);

#endif