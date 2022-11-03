#ifndef MEMORY_HEAP_H
#define MEMORY_HEAP_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct heap_seg_header {
    size_t length;
    struct heap_seg_header *next;
    struct heap_seg_header *last;
    bool free;
} __attribute__((packed)) heap_seg_header_t;

void initialize_heap(void *heap_address, int initial_pages);
void expand_heap(int pages);

void *malloc(size_t size);
void free(void *address);

#endif