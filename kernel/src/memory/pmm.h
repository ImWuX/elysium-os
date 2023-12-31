#pragma once
#include <stdint.h>
#include <stddef.h>
#include <lib/spinlock.h>
#include <lib/list.h>

#define PMM_MAX_ORDER 7

#define PMM_ZONE_COUNT 2
#define PMM_ZONE_AF_SHIFT 6
#define PMM_ZONE_AF_MASK 0b11
#define PMM_ZONE_INDEX_NORMAL 0
#define PMM_ZONE_INDEX_DMA 1

#define PMM_AF_ZERO (1 << 0)
#define PMM_AF_ZONE_DMA (PMM_ZONE_INDEX_DMA << PMM_ZONE_AF_SHIFT)
#define PMM_AF_ZONE_NORMAL (PMM_ZONE_INDEX_NORMAL << PMM_ZONE_AF_SHIFT)

#define PMM_GENERAL (PMM_AF_ZONE_NORMAL)
#define PMM_DMA (PMM_AF_ZONE_DMA)

typedef uint16_t pmm_allocator_flags_t;
typedef uint8_t pmm_order_t;

typedef struct {
    spinlock_t lock;
    list_t regions;
    list_t lists[PMM_MAX_ORDER + 1];
    size_t page_count;
    size_t free_count;
    uintptr_t start;
    uintptr_t end;
    char *name;
} pmm_zone_t;

typedef struct pmm_page {
    list_t list;
    struct pmm_region *region;
    uintptr_t paddr;
    uint8_t order : 3;
    uint8_t free : 1;
} pmm_page_t;

typedef struct pmm_region {
    list_t list;
    pmm_zone_t *zone;
    uintptr_t base;
    size_t page_count;
    size_t free_count;
    pmm_page_t pages[];
} pmm_region_t;

extern pmm_zone_t g_pmm_zones[PMM_ZONE_COUNT];

/**
 * @brief Create a memory zone.
 * @param zone_index
 * @param name
 * @param start
 * @param end
 */
void pmm_zone_create(int zone_index, char *name, uintptr_t start, uintptr_t end);

/**
 * @brief Adds a block of memory to be managed by the PMM.
 * @param zone_index
 * @param base region base address
 * @param size region size in bytes
 */
void pmm_region_add(int zone_index, uintptr_t base, size_t size);

/**
 * @brief Allocates a block of size order^2 pages.
 * @param order
 * @returns first page of the allocated block
 */
pmm_page_t *pmm_alloc(pmm_order_t order, pmm_allocator_flags_t flags);

/**
 * @brief Allocates the smallest block of size N^2 pages to fit size.
 * @param page_count
 * @param flags
 * @returns block of size equal to or larger than page count
 */
pmm_page_t *pmm_alloc_pages(size_t page_count, pmm_allocator_flags_t flags);

/**
 * @brief Allocates a page of memory.
 * @param flags
 * @returns allocated page
 */
pmm_page_t *pmm_alloc_page(pmm_allocator_flags_t flags);

/**
 * @brief Frees a previously allocated page.
 * @param page
 */
void pmm_free(pmm_page_t *page);