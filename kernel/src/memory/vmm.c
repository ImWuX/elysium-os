#include "vmm.h"
#include <stdbool.h>
#include <arch/vmm.h>

static vmm_range_t *find(vmm_address_space_t *address_space, uintptr_t vaddr, size_t size) {
    vmm_range_t *range = address_space->ranges;
    while(range) {
        if(vaddr + size >= range->start && vaddr < range->end) return range;
        range = range->next;
    }
    return 0;
}

int vmm_alloc_wired(vmm_address_space_t *address_space, uintptr_t vaddr, size_t npages, uint64_t flags) {
    for(size_t i = 0; i < npages; i++) {
        pmm_page_t *page = pmm_page_alloc(PMM_PAGE_USAGE_WIRED);
        arch_vmm_map(address_space, vaddr + i * ARCH_PAGE_SIZE, page->paddr, flags);
    }
    return 0;
}

int vmm_alloc(vmm_address_space_t *address_space, uintptr_t vaddr, size_t size) {
    return -1;
}

int vmm_free(vmm_address_space_t *address_space, uintptr_t vaddr, size_t size) {
    return -1;
}