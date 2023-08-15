#include "vmm.h"
#include <arch/vmm.h>
#include <memory/heap.h>

vmm_address_space_t g_kernel_address_space;

static bool is_region_free(vmm_address_space_t *as, uintptr_t base, size_t length) {
    list_t *entry;
    LIST_FOREACH(entry, &as->segments) {
        vmm_segment_t *seg = LIST_GET(entry, vmm_segment_t, list);
        if(base >= seg->base + seg->length) continue;
        if(base + length <= seg->base) continue;
        return false;
    }
    return true;
}

/* @warning Requires address space lock */
static vmm_segment_t *alloc_segment(vmm_address_space_t *as, uintptr_t base, size_t length) {
    if(!is_region_free(as, base, length)) return 0;
    vmm_segment_t *segment = heap_alloc(sizeof(vmm_segment_t));
    segment->address_space = as;
    segment->base = base;
    segment->length = length;
    list_t *entry;
    LIST_FOREACH(entry, &as->segments) {
        vmm_segment_t *lseg = LIST_GET(entry, vmm_segment_t, list);
        if(lseg->base < base) continue;
        list_insert_before(&lseg->list, &segment->list);
        return segment;
    }
    list_insert_before(&as->segments, &segment->list);
    return segment;
}

/* @warning Currently does not actually allocate a segment.. */
int vmm_alloc_wired(vmm_address_space_t *as, uintptr_t vaddr, size_t npages, uint64_t flags) {
    slock_acquire(&as->lock);
    for(size_t i = 0; i < npages; i++) {
        pmm_page_t *page = pmm_alloc_page(ARCH_PAGE_SIZE);
        arch_vmm_map(as, vaddr + i * ARCH_PAGE_SIZE, page->paddr, flags);
    }
    slock_release(&as->lock);
    return 0;
}

int vmm_alloc_direct(vmm_address_space_t *as, uintptr_t vaddr, size_t npages, uintptr_t paddr, uint64_t flags) {
    slock_acquire(&as->lock);
    vmm_segment_t *segment = alloc_segment(as, vaddr, npages * ARCH_PAGE_SIZE);
    if(!segment) return -1;
    for(size_t i = 0; i < npages; i++) {
        arch_vmm_map(as, vaddr + i * ARCH_PAGE_SIZE, paddr + i * ARCH_PAGE_SIZE, flags);
    }
    slock_release(&as->lock);
    return 0;
}