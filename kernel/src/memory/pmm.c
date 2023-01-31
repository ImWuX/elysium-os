#include "pmm.h"
#include <panic.h>
#include <memory/freelist.h>

#define PAGE_SIZE 0x1000

static uint64_t g_total_memory;
static uint64_t g_used_memory;

void pmm_initialize(tartarus_memap_entry_t *memory_map, uint16_t memory_map_length) {
    uintptr_t last_address = 0;
    for(uint16_t i = 1; i <= memory_map_length; i++) {
        tartarus_memap_entry_t entry = memory_map[memory_map_length - i];
        switch(entry.type) {
            case TARTARUS_MEMAP_TYPE_USABLE:
                if(freelist_add_region(entry.base_address, entry.length)) panic("PMM", "Unaligned usable region");
                g_total_memory += entry.length;
                break;
            case TARTARUS_MEMAP_TYPE_KERNEL:
            case TARTARUS_MEMAP_TYPE_BOOT_RECLAIMABLE:
            case TARTARUS_MEMAP_TYPE_ACPI_RECLAIMABLE:
                g_total_memory += entry.length;
                g_used_memory += entry.length;
                break;
            default:;
        }
    }
}

void *pmm_page_request() {
    void *page = freelist_page_request();
    if(!page) panic("PMM", "Out of physical memory");
    g_used_memory += PAGE_SIZE;
    return page;
}

void pmm_page_release(void *page_address) {
    freelist_page_release(page_address);
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