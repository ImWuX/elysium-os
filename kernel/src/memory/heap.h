#ifndef MEMORY_HEAP_H
#define MEMORY_HEAP_H

#include <stdint.h>
#include <stdbool.h>

typedef struct heap_entry {
    uint64_t length;
    bool free;
    struct heap_entry *next;
    struct heap_entry *prev;
} heap_entry_t;

void heap_initialize(void *address, uint64_t initial_pages);
void *heap_alloc(uint64_t count);
void heap_free(void* address);

#endif