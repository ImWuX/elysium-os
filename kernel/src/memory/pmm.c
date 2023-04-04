#include "pmm.h"
#include <string.h>
#include <panic.h>
#include <memory/hhdm.h>
#include <memory/pmm_lowmem.h>

#define PAGE_SIZE 0x1000
#define LOWMEM_SIZE 0x1000000

static uint64_t g_total_memory;
static uint64_t g_free_memory;
static uint64_t g_low_memory;

static uintptr_t g_freelist = 0;

void pmm_initialize(tartarus_memap_entry_t *memory_map, uint16_t memory_map_length) {
    uint64_t lowmem_bitmap_size = (LOWMEM_SIZE / PAGE_SIZE + 7) / 8;
    uintptr_t lowmem_bitmap_address;
    for(uint16_t i = 0; i < memory_map_length; i++) {
        tartarus_memap_entry_t entry = memory_map[i];
        if(entry.type != TARTARUS_MEMAP_TYPE_USABLE) continue;
        if(entry.length < lowmem_bitmap_size) continue;
        pmm_lowmem_initialize(HHDM(entry.base_address), lowmem_bitmap_size);
        lowmem_bitmap_address = entry.base_address;
        break;
    }

    uintptr_t latest_dma_address = 0;
    for(uint16_t i = 0; i < memory_map_length; i++) {
        tartarus_memap_entry_t entry = memory_map[memory_map_length - i - 1];
        if(entry.type == TARTARUS_MEMAP_TYPE_BAD || entry.type == TARTARUS_MEMAP_TYPE_RESERVED) continue;
        g_total_memory += entry.length;
        if(entry.type != TARTARUS_MEMAP_TYPE_USABLE) continue;
        g_free_memory += entry.length;
        for(uintptr_t address = entry.base_address; address < entry.base_address + entry.length; address += PAGE_SIZE) {
            if(address >= lowmem_bitmap_address && address < lowmem_bitmap_address + lowmem_bitmap_size) {
                g_free_memory -= PAGE_SIZE;
                continue;
            }
            if(address < LOWMEM_SIZE) {
                pmm_lowmem_release(address, 1);
                g_low_memory += PAGE_SIZE;
                g_free_memory -= PAGE_SIZE;
            } else {
                *((uintptr_t *) HHDM(address)) = g_freelist;
                g_freelist = (uintptr_t) address;
            }
        }
    }
}

void *pmm_page_request() {
    uintptr_t address = g_freelist;
    g_freelist = *((uintptr_t *) HHDM(address));
    if(!address) panic("PMM", "Out of physical memory");
    g_free_memory -= PAGE_SIZE;
    return (void *) address;
}

void pmm_page_release(void *page_address) {
    *((uintptr_t *) HHDM(page_address)) = g_freelist;
    g_freelist = (uintptr_t) page_address;
    g_free_memory += PAGE_SIZE;
}

uint64_t pmm_mem_total() {
    return g_total_memory;
}

uint64_t pmm_mem_used() {
    return g_total_memory - g_free_memory;
}

uint64_t pmm_mem_free() {
    return g_free_memory;
}

uint64_t pmm_mem_low() {
    return g_low_memory;
}