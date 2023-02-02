#include "bitmap.h"
#include <memory/hhdm.h>

#define PAGE_SIZE 0x1000

static BITMAP_BLOCK_SIZE *g_address;
static uint64_t g_size;

void bitmap_initialize(uintptr_t address, uint64_t size) {
    g_address = (BITMAP_BLOCK_SIZE *) HHDM(address);
    g_size = size;
    for(uint64_t i = 0; i < (g_size + sizeof(BITMAP_BLOCK_SIZE) - 1) / sizeof(BITMAP_BLOCK_SIZE); i++) {
        g_address[i] = BITMAP_BLOCK_FILL;
    }
}

void *bitmap_request() {
    for(uint64_t i = 0; i < g_size; i++) {
        uint64_t block = i / sizeof(BITMAP_BLOCK_SIZE);
        uint64_t block_index = i % sizeof(BITMAP_BLOCK_SIZE);
        if(!(g_address[block] & (1 << block_index))) {
            g_address[block] |= 1 << block_index;
            return (void *) (i * PAGE_SIZE);
        }
    }
    return 0;
}

void bitmap_release(void *address) {
    uint64_t block = (uintptr_t) address / PAGE_SIZE / sizeof(BITMAP_BLOCK_SIZE);
    uint64_t block_index = (uintptr_t) address / PAGE_SIZE % sizeof(BITMAP_BLOCK_SIZE);
    g_address[block] &= ~(1 << block_index);
}