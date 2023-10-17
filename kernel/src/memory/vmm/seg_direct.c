#include "seg_direct.h"
#include <lib/c/errno.h>
#include <memory/vmm.h>
#include <memory/heap.h>
#include <arch/vmm.h>
#include <lib/c/errno.h>

static int direct_map(vmm_segment_t *segment, uintptr_t base, size_t length) {
    uintptr_t offset = base - segment->base;
    for(uintptr_t i = 0; i < length / ARCH_PAGE_SIZE; i++) {
        arch_vmm_map(segment->address_space, base + i * ARCH_PAGE_SIZE, (uintptr_t) segment->data + offset + i * ARCH_PAGE_SIZE, segment->protection);
    }
    return 0;
}

static int direct_unmap(vmm_segment_t *segment [[maybe_unused]], uintptr_t base [[maybe_unused]], size_t length [[maybe_unused]]) {
    return -ENOSYS;
}

static bool direct_fault(vmm_segment_t *segment [[maybe_unused]], uintptr_t address [[maybe_unused]]) {
    return false;
}

static void direct_free(vmm_segment_t *segment) {
    heap_free(segment);
}

static vmm_segment_ops_t ops = {
    .map = &direct_map,
    .unmap = &direct_unmap,
    .fault = &direct_fault,
    .free = &direct_free
};

vmm_segment_t *seg_direct(vmm_address_space_t *as, uintptr_t base, size_t length, int prot, uintptr_t paddr) {
    vmm_segment_t *segment = heap_alloc(sizeof(vmm_segment_t));
    segment->address_space = as;
    segment->base = base;
    segment->length = length;
    segment->protection = prot;
    segment->ops = &ops;
    segment->data = (void *) paddr;
    return segment;
}