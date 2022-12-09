#ifndef PAGING_H
#define PAGING_H

#include <stdint.h>
#include <boot/memap.h>

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

void paging_initialize(boot_memap_entry_t *memory_map, uint64_t memory_map_length);
void paging_map_memory(void *physical_address, void *virtual_address);
void paging_map_memory_2mb(void *physical_address, void *virtual_address);

#endif