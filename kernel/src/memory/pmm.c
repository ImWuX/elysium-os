#include "pmm.h"
#include <panic.h>
#include <memory/hhdm.h>

#define PAGE_SIZE 0x1000
#define DMA_REGION_LENGTH 0x1000000

static uint64_t g_total_memory;
static uint64_t g_used_memory;

static uintptr_t g_freelist = 0;

void pmm_initialize(tartarus_memap_entry_t *memory_map, uint16_t memory_map_length) {
    uintptr_t last_address = 0;
    for(uint16_t i = 0; i < memory_map_length; i++) {
        tartarus_memap_entry_t entry = memory_map[memory_map_length - i - 1];
        switch(entry.type) {
            case TARTARUS_MEMAP_TYPE_USABLE:
                for(uintptr_t address = entry.base_address; address < entry.base_address + entry.length; address += PAGE_SIZE) {
                    pmm_page_release((void *) address);
                }
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
    uintptr_t address = g_freelist;
    g_freelist = *((uintptr_t *) HHDM(address));
    if(!address) panic("PMM", "Out of physical memory");
    g_used_memory += PAGE_SIZE;
    return (void *) address;
}

void pmm_page_release(void *page_address) {
    *((uintptr_t *) HHDM(page_address)) = g_freelist;
    g_freelist = (uintptr_t) page_address;
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