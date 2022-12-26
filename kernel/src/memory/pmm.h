#ifndef MEMORY_PMM_H
#define MEMORY_PMM_H

#include <stdint.h>
#include <tartarus.h>

void pmm_initialize(tartarus_memap_entry_t *memory_map, uint16_t memory_map_length);
void *pmm_page_alloc();
void pmm_page_free(void *page);

#endif