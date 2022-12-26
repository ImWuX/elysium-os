#ifndef MEMORY_VMM_H
#define MEMORY_VMM_H

#include <stdint.h>

typedef struct {
    uint64_t entries[512];
} __attribute__((packed)) vmm_page_table_t;

void vmm_initialize(uint64_t pml4_address);

#endif