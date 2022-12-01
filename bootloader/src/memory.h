#ifndef MEMORY_MEMORY_H
#define MEMORY_MEMORY_H

#include <stdint.h>

extern void *ld_mmap;

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

void initialize_memory();
void map_memory(void *physical_address, void *virtual_address);
void map_memory_2mb(void *physical_address, void *virtual_address);
uint64_t get_pml4_address();

#endif