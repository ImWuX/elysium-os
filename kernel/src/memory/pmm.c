#include "pmm.h"
#include <panic.h>
#include <arch/types.h>
#include <memory/hhdm.h>
#include <lib/slock.h>

#define DIVUP(VAL, DIVISOR) (((VAL) + (DIVISOR) - 1) / (DIVISOR))

static list_t g_regions = LIST_INIT;
static list_t g_lists[PMM_MAX_ORDER + 1] = {};
static slock_t g_lock = SLOCK_INIT;

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

void pmm_region_add(uintptr_t base, size_t size) {
    pmm_region_t *region = (pmm_region_t *) HHDM(base);
    region->base = base;
    region->page_count = size / ARCH_PAGE_SIZE;

    size_t used_pages = DIVUP(sizeof(pmm_region_t) + sizeof(pmm_page_t) * region->page_count, ARCH_PAGE_SIZE);
    region->free_count = region->page_count - used_pages;

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
        uint8_t order = pagecount_to_order(free_pages);
        if(free_pages & (free_pages - 1)) order--;
        if(order > PMM_MAX_ORDER) order = PMM_MAX_ORDER;

        pmm_page_t *page = &region->pages[i];
        page->order = order;
        list_insert_behind(&g_lists[order], &page->list);

        size_t order_size = order_to_pagecount(order);
        free_pages -= order_size;
        i += order_size;
    }

    list_insert_behind(&g_regions, &region->list);
}

pmm_page_t *pmm_alloc(uint8_t order) {
    uint8_t avl_order = order;
    if(avl_order > PMM_MAX_ORDER) panic("PMM", "Invalid order");
    slock_acquire(&g_lock);
    while(list_is_empty(&g_lists[avl_order])) {
        if(++avl_order > PMM_MAX_ORDER) panic("PMM", "Exceeded maximum order (OUT OF MEMORY)");
    }
    pmm_page_t *page = LIST_GET(g_lists[avl_order].next, pmm_page_t, list);
    list_delete(&page->list);
    for(; avl_order > order; avl_order--) {
        pmm_page_t *buddy = &page->region->pages[get_local_pfn(page) + (order_to_pagecount(avl_order - 1))];
        buddy->order = avl_order - 1;
        list_insert_behind(&g_lists[avl_order - 1], &buddy->list);
    }
    slock_release(&g_lock);
    page->order = order;
    page->free = false;
    page->region->free_count -= order_to_pagecount(order);
    return page;
}

pmm_page_t *pmm_alloc_pages(size_t page_count) {
    return pmm_alloc(pagecount_to_order(page_count));
}

pmm_page_t *pmm_alloc_page() {
    return pmm_alloc(0);
}

void pmm_free(pmm_page_t *page) {
    size_t data_base = page->region->base + DIVUP(sizeof(pmm_region_t) + sizeof(pmm_page_t) * page->region->page_count, ARCH_PAGE_SIZE) * ARCH_PAGE_SIZE;
    page->free = true;
    slock_acquire(&g_lock);
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
    list_insert_behind(&g_lists[page->order], &page->list);
    slock_release(&g_lock);
}