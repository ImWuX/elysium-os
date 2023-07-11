#ifndef MEMORY_PMM_H
#define MEMORY_PMM_H

#include <stdint.h>
#include <stddef.h>
#include <lib/list.h>

#define PMM_MAX_ORDER 7

typedef struct pmm_page {
    list_t list;
    struct pmm_region *region;
    uintptr_t paddr;
    uint8_t order : 7;
    uint8_t free : 1;
} pmm_page_t;

typedef struct pmm_region {
    list_t list;
    uintptr_t base;
    size_t page_count;
    size_t free_count;
    pmm_page_t pages[];
} pmm_region_t;

/**
 * @brief Adds a block of memory to be managed by the PMM.
 *
 * @param base Base address of memory block
 * @param size Size of the block in bytes
 */
void pmm_region_add(uintptr_t base, size_t size);

/**
 * @brief Allocates a block of size order^2 pages
 *
 * @param order Block size
 * @return First page of the allocated block
 */
pmm_page_t *pmm_alloc(uint8_t order);

/**
 * @brief Allocates the smallest block of size N^2 pages to fit size
 *
 * @param page_count Page count
 * @return Block size equal to or larger than page count
 */
pmm_page_t *pmm_alloc_pages(size_t page_count);

/**
 * @brief Allocates a page of memory.
 *
 * @returns The allocated page
 */
pmm_page_t *pmm_alloc_page();

/**
 * @brief Frees a previously allocated page.
 *
 * @param page The page/block to be freed
 */
void pmm_free(pmm_page_t *page);

#endif