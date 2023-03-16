#ifndef MEMORY_PMM_H
#define MEMORY_PMM_H

#include <stdint.h>
#include <tartarus.h>

typedef enum {
    PMM_FRAME_FLAGS_DMA = (1 << 1)
} pmm_frame_flags_t;

typedef struct pmm_dma_node {
    struct pmm_dma_node *next;
    int length;
} pmm_dma_node_t;

void pmm_initialize(tartarus_memap_entry_t *memory_map, uint16_t memory_map_length);
void *pmm_page_request();
void pmm_page_release(void *page_address);
void *pmm_page_request_dma(int count);

uint64_t pmm_mem_total();
uint64_t pmm_mem_used();
uint64_t pmm_mem_free();

#endif