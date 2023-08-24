#include "pmm.h"
#include <string.h>
#include <lib/assert.h>
#include <lib/panic.h>
#include <arch/types.h>
#include <memory/hhdm.h>
#include <lib/slock.h>

#define DIVUP(VAL, DIVISOR) (((VAL) + (DIVISOR) - 1) / (DIVISOR))

pmm_zone_t g_pmm_zones[PMM_ZONE_COUNT];

static inline size_t get_local_pfn(pmm_page_t *page) {
    return (page->paddr - page->region->base) / ARCH_PAGE_SIZE;
}

static inline uint8_t pagecount_to_order(size_t pages) {
    if(pages == 1) return 0;
    return (uint8_t) ((sizeof(unsigned long long) * 8) - __builtin_clzll(pages - 1));
}

static inline size_t order_to_pagecount(uint8_t order) {
    return (size_t) 1 << order;
}

void pmm_zone_create(int zone_index, char *name, uintptr_t start, uintptr_t end) {
    memset(&g_pmm_zones[zone_index], 0, sizeof(pmm_zone_t));
    g_pmm_zones[zone_index].lock = SLOCK_INIT;
    g_pmm_zones[zone_index].regions = LIST_INIT;
    for(int i = 0; i <= PMM_MAX_ORDER; i++) g_pmm_zones[zone_index].lists[i] = LIST_INIT;
    g_pmm_zones[zone_index].name = name;
    g_pmm_zones[zone_index].start = start;
    g_pmm_zones[zone_index].end = end;
}

void pmm_region_add(int zone_index, uintptr_t base, size_t size) {
    pmm_region_t *region = (pmm_region_t *) HHDM(base);
    region->zone = &g_pmm_zones[zone_index];
    region->base = base;
    region->page_count = size / ARCH_PAGE_SIZE;
    region->zone->page_count += region->page_count;

    size_t used_pages = DIVUP(sizeof(pmm_region_t) + sizeof(pmm_page_t) * region->page_count, ARCH_PAGE_SIZE);
    region->free_count = region->page_count - used_pages;
    region->zone->free_count += region->free_count;

    for(size_t i = 0; i < region->page_count; i++) {
        region->pages[i] = (pmm_page_t) {
            .region = region,
            .free = true,
            .paddr = region->base + i * ARCH_PAGE_SIZE
        };
    }

    for(size_t i = 0; i < used_pages; i++) {
        region->pages[i].free = false;
    }

    for(size_t i = used_pages, free_pages = region->free_count; free_pages;) {
        pmm_order_t order = pagecount_to_order(free_pages);
        if(free_pages & (free_pages - 1)) order--;
        if(order > PMM_MAX_ORDER) order = PMM_MAX_ORDER;

        pmm_page_t *page = &region->pages[i];
        page->order = order;
        list_insert_behind(&region->zone->lists[order], &page->list);

        size_t order_size = order_to_pagecount(order);
        free_pages -= order_size;
        i += order_size;
    }

    list_insert_behind(&region->zone->regions, &region->list);
}

pmm_page_t *pmm_alloc(pmm_order_t order, pmm_allocator_flags_t flags) {
    ASSERT(order <= PMM_MAX_ORDER);
    pmm_order_t avl_order = order;
    pmm_zone_t *zone = &g_pmm_zones[(flags >> PMM_ZONE_AF_SHIFT) & PMM_ZONE_AF_MASK];
    slock_acquire(&zone->lock);
    while(list_is_empty(&zone->lists[avl_order])) {
        ASSERT(++avl_order <= PMM_MAX_ORDER, "Out of memory");
    }
    pmm_page_t *page = LIST_GET(zone->lists[avl_order].next, pmm_page_t, list);
    list_delete(&page->list);
    for(; avl_order > order; avl_order--) {
        pmm_page_t *buddy = &page->region->pages[get_local_pfn(page) + (order_to_pagecount(avl_order - 1))];
        buddy->order = avl_order - 1;
        list_insert_behind(&zone->lists[avl_order - 1], &buddy->list);
    }
    slock_release(&zone->lock);
    page->order = order;
    page->free = false;
    page->region->free_count -= order_to_pagecount(order);
    page->region->zone->free_count -= order_to_pagecount(order);
    if(flags & PMM_AF_ZERO) memset((void *) HHDM(page->paddr), 0, order_to_pagecount(order) * ARCH_PAGE_SIZE);
    return page;
}

pmm_page_t *pmm_alloc_pages(size_t page_count, pmm_allocator_flags_t flags) {
    return pmm_alloc(pagecount_to_order(page_count), flags);
}

pmm_page_t *pmm_alloc_page(pmm_allocator_flags_t flags) {
    return pmm_alloc(0, flags);
}

void pmm_free(pmm_page_t *page) {
    size_t data_base = page->region->base + DIVUP(sizeof(pmm_region_t) + sizeof(pmm_page_t) * page->region->page_count, ARCH_PAGE_SIZE) * ARCH_PAGE_SIZE;
    page->free = true;
    page->region->free_count += order_to_pagecount(page->order);
    page->region->zone->free_count += order_to_pagecount(page->order);
    pmm_zone_t *zone = page->region->zone;
    slock_acquire(&zone->lock);
    for(;;) {
        if(page->order >= PMM_MAX_ORDER) break;
        size_t buddy_addr = data_base + ((page->paddr - data_base) ^ (order_to_pagecount(page->order) * ARCH_PAGE_SIZE));
        if(buddy_addr >= page->region->base + page->region->page_count * ARCH_PAGE_SIZE) break;
        pmm_page_t *buddy = &page->region->pages[(buddy_addr - page->region->base) / ARCH_PAGE_SIZE];
        if(!buddy->free || buddy->order != page->order) break;

        list_delete(&buddy->list);
        buddy->order++;
        page->order++;
        if(buddy->paddr < page->paddr) page = buddy;
    }
    list_insert_behind(&zone->lists[page->order], &page->list);
    slock_release(&zone->lock);
}