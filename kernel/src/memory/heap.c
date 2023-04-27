#include "heap.h"
#include <stdbool.h>
#include <panic.h>
#include <arch/types.h>
#include <memory/pmm.h>

#define MIN_SPLIT_SIZE 8
#define INITIAL_PAGES 5

typedef struct heap_entry {
    uint64_t length;
    bool free;
    struct heap_entry *next;
    struct heap_entry *prev;
} heap_entry_t;

static uintptr_t g_heap_start;
static uintptr_t g_heap_end;
static size_t g_heap_size;

static vmm_address_space_t *g_address_space;

static heap_entry_t *g_tail;
static heap_entry_t *g_head;

static void expand(size_t npages) {
    if(g_heap_start + g_heap_size + npages * ARCH_PAGE_SIZE >= g_heap_end) panic("HHDM", "Heap is full");
    int r = vmm_alloc_wired(g_address_space, g_heap_start + g_heap_size, npages, VMM_DEFAULT_KERNEL_FLAGS);
    if(r < 0) panic("HEAP", "Failed to allocate wired memory");
    if(g_tail && g_tail->free) {
        g_tail->length += npages * ARCH_PAGE_SIZE;
    } else {
        heap_entry_t *new = (heap_entry_t *) (g_heap_start + g_heap_size);
        new->free = true;
        new->length = (npages * ARCH_PAGE_SIZE) - sizeof(heap_entry_t);
        new->prev = g_tail;
        if(g_tail) new->next = g_tail->next;
        else new->next = 0;
        if(g_tail) g_tail->next = new;
        g_tail = new;
    }
    g_heap_size += npages * ARCH_PAGE_SIZE;
}

void heap_initialize(vmm_address_space_t *address_space, uintptr_t start, uintptr_t end) {
    g_address_space = address_space;
    g_heap_start = start;
    g_heap_end = end;
    g_heap_size = 0;
    g_tail = 0;
    expand(INITIAL_PAGES);
    g_head = g_tail;
}

void *heap_alloc(size_t size) {
    heap_entry_t *current_entry = g_head;
    while(current_entry) {
        if(current_entry->free && current_entry->length >= size) {
            if(current_entry->length >= size + MIN_SPLIT_SIZE + sizeof(heap_entry_t)) {
                heap_entry_t *old = current_entry->next;
                heap_entry_t *new = (heap_entry_t *) ((uintptr_t) current_entry + sizeof(heap_entry_t) + size);
                current_entry->next = new;
                new->next = old;
                new->prev = current_entry;
                if(old) old->prev = new;
                new->free = true;
                new->length = current_entry->length - size - sizeof(heap_entry_t);
                current_entry->length = size;
                if(current_entry == g_tail) g_tail = new;
            }
            current_entry->free = false;
            return (void *) current_entry + sizeof(heap_entry_t);
        }
        current_entry = current_entry->next;
    }
    expand((size + sizeof(heap_entry_t)) / ARCH_PAGE_SIZE + 1);
    return heap_alloc(size);
}

void heap_free(void* address) {
    heap_entry_t *entry = (heap_entry_t *) (address - sizeof(heap_entry_t));
    entry->free = true;
    heap_entry_t *next_entry = entry->next;
    if(next_entry && next_entry->free) {
        entry->next = next_entry->next;
        if(next_entry->next) next_entry->next->prev = entry;
        entry->length += next_entry->length + sizeof(heap_entry_t);
    }
    heap_entry_t *prev_entry = entry->prev;
    if(prev_entry && prev_entry->free) {
        prev_entry->next = entry->next;
        if(entry->next) entry->next->prev = prev_entry;
        prev_entry->length += entry->length + sizeof(heap_entry_t);
    }
}