#include "seg_anon.h"
#include <memory/vmm.h>
#include <memory/heap.h>
#include <arch/vmm.h>
#include <lib/c/errno.h>

typedef struct {
    bool wired;
} anon_data_t;

static int anon_map(vmm_segment_t *segment, uintptr_t base, size_t length) {
    for(uintptr_t i = 0; i < length / ARCH_PAGE_SIZE; i++) {
        arch_vmm_map(segment->address_space, base + ARCH_PAGE_SIZE * i, pmm_alloc_page(PMM_GENERAL | PMM_AF_ZERO)->paddr, segment->protection);
    }
    return 0;
}

static int anon_unmap(vmm_segment_t *segment [[maybe_unused]], uintptr_t base [[maybe_unused]], size_t length [[maybe_unused]]) {
    return -ENOSYS;
}

static bool anon_fault(vmm_segment_t *segment [[maybe_unused]], uintptr_t address [[maybe_unused]]) {
    return false;
}

static void anon_free(vmm_segment_t *segment) {
    heap_free(segment->data);
    heap_free(segment);
}

static vmm_segment_ops_t ops = {
    .map = &anon_map,
    .unmap = &anon_unmap,
    .fault = &anon_fault,
    .free = &anon_free
};

// CRITICAL: DO COALESCING!!!
// static bool can_coalesce(vmm_segment_t *segment) {
//     vmm_segment_t *prev = NULL;
//     vmm_segment_t *next = NULL;
//     list_t *entry;
//     LIST_FOREACH(entry, &as->segments) {
//         vmm_segment_t *other = LIST_GET(entry, vmm_segment_t, list);
//         if(other->base < base) {
//             prev = other;
//             continue;
//         }
//         next = other;
//         break;
//     }

//     /* This coalescing only works for wired/anon segments afaik, atleast vfs and direct ones need more checks etc*/
//     vmm_segment_t *segment = NULL;
//     if(
//         prev && prev->type == VMM_TYPE_ANON && prev->protection == prot && prev->type_specific.wired == wired &&
//         prev->base / ARCH_PAGE_SIZE + prev->length / ARCH_PAGE_SIZE == base / ARCH_PAGE_SIZE
//     ) {
//         segment = prev;
//         segment->length += length;
//     }

//     if(
//         next && next->type == VMM_TYPE_ANON && next->protection == prot && next->type_specific.wired == wired &&
//         next->base / ARCH_PAGE_SIZE == base / ARCH_PAGE_SIZE + length / ARCH_PAGE_SIZE
//     ) {
//         if(segment) {
//             segment += next->length;
//             list_delete(&next->list);
//             heap_free(segment);
//         } else {
//             next->length += length;
//             next->base -= length;
//         }
//     }
// }

vmm_segment_t *seg_anon(vmm_address_space_t *as, uintptr_t base, size_t length, int prot, bool wired) {
    anon_data_t *data = heap_alloc(sizeof(anon_data_t));
    data->wired = wired;
    vmm_segment_t *segment = heap_alloc(sizeof(vmm_segment_t));
    segment->address_space = as;
    segment->base = base;
    segment->length = length;
    segment->protection = prot;
    segment->ops = &ops;
    segment->data = (void *) data;
    return segment;
}