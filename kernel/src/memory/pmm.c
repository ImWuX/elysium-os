#include "pmm.h"
#include <string.h>
#include <panic.h>
#include <memory/hhdm.h>

#define PAGE_SIZE 0x1000
#define DMA_REGION_LENGTH 0x1000000

static uint64_t g_total_memory;
static uint64_t g_free_memory;

static uintptr_t g_freelist = 0;
static pmm_dma_node_t *g_dma_nodelist = 0;

void pmm_initialize(tartarus_memap_entry_t *memory_map, uint16_t memory_map_length) {
    uintptr_t latest_dma_address = 0;
    for(uint16_t i = 0; i < memory_map_length; i++) {
        tartarus_memap_entry_t entry = memory_map[memory_map_length - i - 1];
        if(entry.type == TARTARUS_MEMAP_TYPE_BAD || entry.type == TARTARUS_MEMAP_TYPE_RESERVED) continue;
        g_total_memory += entry.length;
        if(entry.type != TARTARUS_MEMAP_TYPE_USABLE) continue;
        g_free_memory += entry.length;
        if(entry.base_address < DMA_REGION_LENGTH && (!g_dma_nodelist || entry.base_address != latest_dma_address + PAGE_SIZE)) {
            pmm_dma_node_t *new_node = (pmm_dma_node_t *) HHDM(entry.base_address);
            memset(new_node, 0, sizeof(pmm_dma_node_t));
            new_node->next = g_dma_nodelist;
            g_dma_nodelist = entry.base_address;
        }
        for(uintptr_t address = entry.base_address; address < entry.base_address + entry.length; address += PAGE_SIZE) {
            if(address < DMA_REGION_LENGTH) {
                g_dma_nodelist->length++;
                latest_dma_address = address;
            } else {
                *((uintptr_t *) HHDM(address)) = g_freelist;
                g_freelist = (uintptr_t) address;
            }
        }
    }
}

void *pmm_page_request_dma(int count) {
    if(!g_dma_nodelist) panic("PMM", "Out of DMA physical memory");
    uintptr_t node_address = g_dma_nodelist;
    pmm_dma_node_t *node = HHDM(node_address);
    pmm_dma_node_t *prev = 0;
    while(node->length < count) {
        if(!node->next) panic("PMM", "Out of DMA physical memory");
        prev = node;
        node_address = node->next;
        node = (pmm_dma_node_t *) HHDM(node_address);
    }
    if(node->length > count) {
        node->length -= count;
        return (void *) node_address + node->length * PAGE_SIZE;
    } else {
        if(prev) prev->next = node->next;
        else g_dma_nodelist = 0;
        return node_address;
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