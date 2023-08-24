#include "heap.h"
#include <lib/assert.h>
#include <lib/panic.h>
#include <lib/list.h>
#include <errno.h>
#include <arch/types.h>
#include <memory/pmm.h>

#define MIN_SPLIT_SIZE 8
#define INITIAL_PAGES 5

typedef struct heap_entry {
    uint64_t length;
    bool free;
    list_t list;
} heap_entry_t;

static_assert(sizeof(heap_entry_t) < ARCH_PAGE_SIZE, "ARCH_PAGE_SIZE is smaller than a heap entry");

static uintptr_t g_heap_start;
static uintptr_t g_heap_end;
static size_t g_heap_size;

static vmm_address_space_t *g_address_space;

static list_t g_heap = LIST_INIT_CIRCULAR(g_heap);

static void expand(size_t npages) {
    ASSERT(g_heap_start + g_heap_size + npages * ARCH_PAGE_SIZE >= g_heap_end);
    int r = vmm_alloc_wired(g_address_space, g_heap_start + g_heap_size, npages, VMM_DEFAULT_KERNEL_FLAGS);
    ASSERT(r < 0);
    if(!list_is_empty(&g_heap) && LIST_GET(g_heap.prev, heap_entry_t, list)->free) {
        LIST_GET(g_heap.prev, heap_entry_t, list)->length += npages * ARCH_PAGE_SIZE;
    } else {
        heap_entry_t *new = (heap_entry_t *) (g_heap_start + g_heap_size);
        new->free = true;
        new->length = (npages * ARCH_PAGE_SIZE) - sizeof(heap_entry_t);
        list_insert_before(&g_heap, &new->list);
    }
    g_heap_size += npages * ARCH_PAGE_SIZE;
}

void heap_initialize(vmm_address_space_t *address_space, uintptr_t start, uintptr_t end) {
    g_address_space = address_space;
    g_heap_start = start;
    g_heap_end = end;
    g_heap_size = 0;
    expand(INITIAL_PAGES);
}

void *heap_alloc_align(size_t size, size_t alignment) {
    list_t *list_entry;
    LIST_FOREACH(list_entry, &g_heap) {
        heap_entry_t *current_entry = LIST_GET(list_entry, heap_entry_t, list);

        if(!current_entry->free) continue;
        uintptr_t alignment_mod = ((uintptr_t) current_entry + sizeof(heap_entry_t)) % alignment;
        uintptr_t alignment_offset = alignment - alignment_mod;
        if(alignment_mod == 0) alignment_offset = 0;
        if(current_entry->length < size + alignment_offset) continue;
        if(alignment_offset == 0) goto alloc;
        if(alignment_offset < sizeof(heap_entry_t) + MIN_SPLIT_SIZE) continue;

        heap_entry_t *new_entry = (heap_entry_t *) ((uintptr_t) current_entry + alignment_offset);
        list_insert_before(&current_entry->list, &new_entry->list);
        new_entry->free = false;
        new_entry->length = current_entry->length - alignment_offset;
        current_entry->length = alignment_offset - sizeof(heap_entry_t);
        current_entry = new_entry;

        alloc:
        if(current_entry->length >= size + MIN_SPLIT_SIZE + sizeof(heap_entry_t)) {
            heap_entry_t *new = (heap_entry_t *) ((uintptr_t) current_entry + sizeof(heap_entry_t) + size);
            list_insert_behind(&current_entry->list, &new->list);
            new->free = true;
            new->length = current_entry->length - size - sizeof(heap_entry_t);
            current_entry->length = size;
        }
        current_entry->free = false;
        return (void *) current_entry + sizeof(heap_entry_t);
    }
    expand((size + sizeof(heap_entry_t)) / ARCH_PAGE_SIZE + 1);
    return heap_alloc_align(size, alignment);
}

void *heap_alloc(size_t size) {
    return heap_alloc_align(size, 1);
}

void heap_free(void* address) {
    heap_entry_t *entry = (heap_entry_t *) (address - sizeof(heap_entry_t));
    entry->free = true;
    if(entry->list.next != &g_heap) {
        heap_entry_t *next_entry = LIST_GET(entry->list.next, heap_entry_t, list);
        if(next_entry->free) {
            entry->length += next_entry->length + sizeof(heap_entry_t);
            list_delete(&next_entry->list);
        }
    }
    if(entry->list.prev != &g_heap) {
        heap_entry_t *prev_entry = LIST_GET(entry->list.prev, heap_entry_t, list);
        if(prev_entry->free) {
            prev_entry->length += entry->length + sizeof(heap_entry_t);
            list_delete(&prev_entry->list);
        }
    }
}