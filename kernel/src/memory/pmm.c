#include "pmm.h"
#include <memory/hhdm.h>

#define PAGE_SIZE 0x1000

static uint64_t g_freelist;
static uint64_t g_total_memory;
static uint64_t g_used_memory;

void pmm_initialize(tartarus_memap_entry_t *memory_map, uint16_t memory_map_length) {
    uint64_t last_address = 0;
    for(uint16_t i = 1; i <= memory_map_length; i++) {
        tartarus_memap_entry_t entry = memory_map[memory_map_length - i];
        if(entry.type != TARTARUS_MEMAP_TYPE_USABLE) continue;

        for(uint64_t j = PAGE_SIZE; j <= entry.length; j += PAGE_SIZE) {
            uint64_t address = (entry.base_address + entry.length) - j;
            *((uint64_t *) HHDM(address)) = last_address;
            last_address = address;
            g_total_memory += PAGE_SIZE;
        }
    }
    g_freelist = last_address;
}

void *pmm_page_alloc() {
    uint64_t address = g_freelist;
    g_freelist = *((uint64_t *) HHDM(address));
    g_used_memory += PAGE_SIZE;
    return (void *) address;
}

void pmm_page_free(void *page_address) {
    *((uint64_t *) HHDM(page_address)) = g_freelist;
    g_freelist = (uint64_t) page_address;
    g_used_memory -= PAGE_SIZE;
}

uint64_t pmm_mem_total() {
    return g_total_memory;
}

uint64_t pmm_mem_used() {
    return g_used_memory;
}

uint64_t pmm_mem_free() {
    return g_total_memory - g_used_memory;
}