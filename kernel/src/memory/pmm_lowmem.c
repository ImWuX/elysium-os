#include "pmm_lowmem.h"
#include <panic.h>
#include <memory/hhdm.h>
#include <stdbool.h>
#include <string.h>

#define PAGE_SIZE 0x1000
#define BLOCK_SIZE (sizeof(bitmap_block_t) * 8)

typedef uint64_t bitmap_block_t;

static bitmap_block_t *g_bitmap;
static uint64_t g_bitmap_size;

static void set(uint64_t index, uint64_t count, bool free) {
    for(uint64_t i = index; i < index + count; i++) {
        uint64_t block = i / BLOCK_SIZE;
        uint64_t mask = (uint64_t) 1 << (i % BLOCK_SIZE);
        if(free) g_bitmap[block] |= mask;
        else g_bitmap[block] &= ~mask;
    }
}

void pmm_lowmem_initialize(void *bitmap_address, uint64_t size) {
    memset(bitmap_address, 0, size);
    g_bitmap = (bitmap_block_t *) bitmap_address;
    g_bitmap_size = size;
}

void *pmm_lowmem_request(unsigned int count) {
    uint64_t current_index = 0;
    uint64_t current_length = 0;
    for(uint64_t i = 0; i < g_bitmap_size; i++) {
        uint64_t block = i / BLOCK_SIZE;
        uint64_t mask = (uint64_t) 1 << (i % BLOCK_SIZE);
        if(g_bitmap[block] & mask) {
            if(++current_length == count) {
                set(current_index, current_length, false);
                return (void *) (current_index * PAGE_SIZE);
            }
        } else {
            current_index = i + 1;
            current_length = 0;
        }
    }
    panic("PMM_LOW", "Out of low memory");
}

void pmm_lowmem_release(void *address, unsigned int count) {
    set((uintptr_t) address / PAGE_SIZE, count, true);
}
