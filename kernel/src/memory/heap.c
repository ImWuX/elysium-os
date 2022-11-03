#include "heap.h"
#include <memory/vmm.h>
#include <memory/pmm.h>

void *heap_start;
void *heap_end;
heap_seg_header_t *heap_end_header;

void initialize_heap(void *heap_address, int initial_pages) {
    heap_start = heap_address;
    heap_end = heap_address;
    heap_end_header = 0;

    expand_heap(initial_pages);
}

void expand_heap(int pages) {
    void *position = heap_end;
    for(int i = 0; i < pages; i++) {
        map_memory(request_page(), position);
        position += 0x1000;
    }

    if(!heap_end_header || !heap_end_header->free) {
        heap_seg_header_t *new_segment = (heap_seg_header_t *) heap_end;
        new_segment->length = pages * 0x1000 - sizeof(heap_seg_header_t);
        new_segment->next = 0;
        new_segment->last = heap_end_header;
        new_segment->free = true;

        if(heap_end_header) heap_end_header->next = new_segment;

        heap_end_header = new_segment;
    } else {
        heap_end_header->length += pages * 0x1000;
    }
    heap_end = position;
}

static void *get_segment(size_t size) {
    heap_seg_header_t *current_header = (heap_seg_header_t *) heap_start;
    while(current_header) {
        if(!current_header->free || current_header->length < size) {
            current_header = current_header->next;
            continue;
        }
        if(current_header->length < size + sizeof(heap_seg_header_t) + 1) {
            current_header->free = false;
            return (void *) ((uint64_t) current_header + sizeof(heap_seg_header_t));
        } else {
            heap_seg_header_t *new_header = (heap_seg_header_t *) ((uint64_t) current_header + sizeof(heap_seg_header_t) + size);
            new_header->length = current_header->length - size - sizeof(heap_seg_header_t);
            new_header->free = true;
            new_header->next = current_header->next;
            new_header->last = current_header;

            current_header->length = size;
            current_header->next = new_header;
            current_header->free = false;

            if(current_header == heap_end_header) heap_end_header = new_header;

            return (void *) ((uint64_t) current_header + sizeof(heap_seg_header_t));
        }
        current_header = current_header->next;
    }
    return 0;
}

void *malloc(size_t size) {
    if(size % 0x10 > 0) {
        size -= size % 0x10;
        size += 0x10;
    }
    if(size == 0) return 0;

    void *ptr = get_segment(size);
    if(!ptr) {
        expand_heap((size + sizeof(heap_seg_header_t)) / 0x1000 + 1);
        ptr = get_segment(size);
    }
    return ptr;
}

void combine_segments(heap_seg_header_t *first, heap_seg_header_t *second) {
    first->length += second->length + sizeof(heap_seg_header_t);
    if(second->next) second->next->last = first;
    first->next = second->next;
}

void free(void *address) {
    heap_seg_header_t *header = (heap_seg_header_t *) ((uint64_t) address - sizeof(heap_seg_header_t));
    header->free = true;
    if(header->next && header->next->free) combine_segments(header, header->next);
    if(header->last && header->last->free) combine_segments(header->last, header);
}