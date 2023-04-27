#include "pmm.h"
#include <panic.h>
#include <arch/types.h>
#include <memory/hhdm.h>

#define CEIL(val, div) (((val) + (div) - 1) / (div))

typedef struct pmm_region {
    struct pmm_region *next;
    uintptr_t base;
    size_t page_count;
    pmm_page_t pages[];
} pmm_region_t;

static pmm_region_t *g_regions = 0;
static pmm_stats_t g_stats = {0};

static pmm_page_t *g_pages_free = 0;

void pmm_region_add(uintptr_t base, size_t size) {
    pmm_region_t *region = (pmm_region_t *) HHDM(base);
    region->base = base;
    region->page_count = size / ARCH_PAGE_SIZE;

    for(size_t i = 0; i < region->page_count; i++) {
        region->pages[i].paddr = region->base + i * ARCH_PAGE_SIZE;
    }

    size_t used_pages = CEIL(sizeof(pmm_region_t) + sizeof(pmm_page_t) * region->page_count, ARCH_PAGE_SIZE);
    size_t i = 0;
    for(; i < used_pages; i++) {
        region->pages[i].usage = PMM_PAGE_USAGE_WIRED;
        g_stats.wired_pages++;
    }

    for(; i < region->page_count; i++) {
        region->pages[i].usage = PMM_PAGE_USAGE_FREE;
        region->pages[i].next = g_pages_free;
        g_pages_free = &region->pages[i];
        g_stats.free_pages++;
    }

    region->next = g_regions;
    g_regions = region;
}

pmm_page_t *pmm_page_alloc(pmm_page_usage_t usage) {
    pmm_page_t *page = g_pages_free;
    if(!page) panic("PMM", "Out of physical memory");
    switch(usage) {
        case PMM_PAGE_USAGE_ANON:
            g_stats.anon_pages++;
            break;
        case PMM_PAGE_USAGE_VMM:
        case PMM_PAGE_USAGE_WIRED:
            g_stats.wired_pages++;
            break;
        case PMM_PAGE_USAGE_BACKED:
            g_stats.backed_pages++;
            break;
        default: panic("PMM", "Invalid usage for alloc");
    }
    g_stats.free_pages--;
    g_pages_free = page->next;
    page->usage = usage;
    page->next = 0;
    page->state = PMM_PAGE_STATE_WIRED;
    return page;
}

void pmm_page_free(pmm_page_t *page) {
    switch(page->usage) {
        case PMM_PAGE_USAGE_ANON:
            g_stats.anon_pages--;
            break;
        case PMM_PAGE_USAGE_VMM:
        case PMM_PAGE_USAGE_WIRED:
            g_stats.wired_pages--;
            break;
        case PMM_PAGE_USAGE_BACKED:
            g_stats.backed_pages--;
            break;
        default: panic("PMM", "Cannot free a free page");
    }
    g_stats.free_pages++;
    page->usage = PMM_PAGE_USAGE_FREE;
    page->next = g_pages_free;
    g_pages_free = page;
}

pmm_stats_t *pmm_stats() {
    return &g_stats;
}