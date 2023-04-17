#ifndef MEMORY_PMM_H
#define MEMORY_PMM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    PMM_PAGE_USAGE_FREE,
    PMM_PAGE_USAGE_WIRED,
    PMM_PAGE_USAGE_ANON,
    PMM_PAGE_USAGE_BACKED
} pmm_page_usage_t;

typedef enum {
    PMM_PAGE_STATE_WIRED
} pmm_page_state_t;

typedef struct pmm_page {
    struct pmm_page *next;
    uintptr_t paddr;
    pmm_page_usage_t usage : 3;
    pmm_page_state_t state : 2;
    int rsv0 : 3;
} pmm_page_t;

typedef struct {
    size_t free_pages;
    size_t wired_pages;
    size_t anon_pages;
    size_t backed_pages;
} pmm_stats_t;

/**
 * @brief Adds a block of memory to be managed by the PMM.
 * 
 * @param base Base address of memory block
 * @param length Size of the block in bytes
 */
void pmm_region_add(uintptr_t base, size_t length);

/**
 * @brief Allocates a page of memory.
 *
 * @param usage What the page will be used for
 * @returns The allocated page
 */
pmm_page_t *pmm_page_alloc(pmm_page_usage_t usage);

/**
 * @brief Frees a previously allocated page.
 *
 * @param page The page to be free'd
 */
void pmm_page_free(pmm_page_t *page);

/**
 * @brief Returns statistics for physical memory
 * 
 * @returns The structure containing the stats
 */
pmm_stats_t *pmm_stats();

#endif