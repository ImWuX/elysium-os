#include "pmm.h"
#include <stdio.h>

#define PAGE_SIZE 0x1000

extern uint64_t g_hhdm;

static uint64_t g_freelist;

void pmm_initialize(tartarus_memap_entry_t *memory_map, uint16_t memory_map_length) {
    uint64_t last_address = 0;
    for(uint16_t i = 1; i <= memory_map_length; i++) {
        tartarus_memap_entry_t entry = memory_map[memory_map_length - i];
        if(entry.type != TARTARUS_MEMAP_TYPE_USABLE) continue;

        for(uint64_t j = PAGE_SIZE; j <= entry.length; j += PAGE_SIZE) {
            uint64_t address = (entry.base_address + entry.length) - j;
            *((uint64_t *) (g_hhdm + address)) = last_address;
            last_address = address;
        }
    }
}

void *pmm_page_alloc() {
    uint64_t address = g_freelist;
    g_freelist = *((uint64_t *) g_hhdm + address);
    return (void *) address;
}

void pmm_page_free(void *page) {
    *((uint64_t *) page) = g_freelist;
    g_freelist = (uint64_t) page;
}