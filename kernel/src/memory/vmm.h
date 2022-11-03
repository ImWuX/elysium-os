#ifndef MEMORY_VMM_H
#define MEMORY_VMM_H

#include <stdint.h>

#define HHDM_BASE 0xFFFF800000000000

typedef struct {
    uint64_t entries[512];
} __attribute__((packed)) page_table_t;

typedef enum {
    PT_FLAG_PRESENT = 0,
    PT_FLAG_READ_WRITE,
    PT_FLAG_USER_SUPERVISOR,
    PT_FLAG_WRITE_THROUGH,
    PT_FLAG_CACHE_DISABLED,
    PT_FLAG_ACCESSED
} pt_entry_flags_t;

void initialize_memory(uint64_t pml4_address);
void map_memory(void *physical_address, void *virtual_address);
uint64_t get_physical_address(uint64_t virtual_address);
uint64_t get_hhdm_address(uint64_t physical_address);

#endif