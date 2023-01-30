#include "heap.h"
#include <memory/pmm.h>
#include <memory/vmm.h>

#define PAGE_SIZE 0x1000
#define MIN_ENTRY_SIZE 8

static void *g_heap_start;
static void *g_heap_end;
static heap_entry_t *tail;
static heap_entry_t *head;

static void expand(uint64_t pages) {
    for(uint64_t i = 0; i < pages; i++) {
        void* page = pmm_page_alloc();
        vmm_map(page, g_heap_end);
        if(tail->free) {
            tail->length += PAGE_SIZE;
        } else {
            heap_entry_t *new = g_heap_end;
            new->free = true;
            new->length = PAGE_SIZE;
            new->prev = tail;
            new->next = tail->next;
            tail->next = new;
            tail = new;
        }
        g_heap_end += PAGE_SIZE;
    }
}

void heap_initialize(void *address, uint64_t initial_pages) {
    g_heap_start = address;
    g_heap_end = address;
    for(uint64_t i = 0; i < initial_pages; i++) {
        void* page = pmm_page_alloc();
        vmm_map(page, g_heap_end);
        g_heap_end += PAGE_SIZE;
    }
    head = g_heap_start;
    head->length = g_heap_end - g_heap_start;
    head->free = true;
    head->next = 0;
    head->prev = 0;
    tail = head;
}

void *heap_alloc(uint64_t count) {
    heap_entry_t *current_entry = head;
    while(current_entry) {
        if(current_entry->free && current_entry->length >= count) {
            if(current_entry->length >= count + MIN_ENTRY_SIZE + sizeof(heap_entry_t)) {
                heap_entry_t *old = current_entry->next;
                heap_entry_t *new = (heap_entry_t *) ((uintptr_t) current_entry + count + sizeof(heap_entry_t));
                current_entry->next = new;
                new->next = old;
                new->prev = current_entry;
                if(old) old->prev = new;
                new->free = true;
                new->length = current_entry->length - count - sizeof(heap_entry_t);
                current_entry->length = count;
                current_entry->free = false;
                if(current_entry == tail) tail = new;
            }
            return (void *) current_entry + sizeof(heap_entry_t);
        }
        current_entry = current_entry->next;
    }
    expand((count + sizeof(heap_entry_t)) / PAGE_SIZE + 1);
    return heap_alloc(count);
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