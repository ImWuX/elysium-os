#include "vmm.h"
#include <arch/vmm.h>
#include <memory/heap.h>

int vmm_alloc_wired(vmm_address_space_t *address_space, uintptr_t vaddr, size_t npages, uint64_t flags) {
    for(size_t i = 0; i < npages; i++) {
        pmm_page_t *page = pmm_alloc_page(PMM_AF_NORMAL);
        arch_vmm_map(address_space, vaddr + i * ARCH_PAGE_SIZE, page->paddr, flags);
    }
    return 0;
}

int vmm_map_direct(vmm_address_space_t *address_space, uintptr_t vaddr, uintptr_t paddr, size_t size) {
    for(size_t i = 0; i < size; i += ARCH_PAGE_SIZE) {
        arch_vmm_map(address_space, vaddr + i, paddr + i, VMM_DEFAULT_KERNEL_FLAGS);
    }

    vmm_range_t *range = heap_alloc(sizeof(vmm_range_t));
    range->start = vaddr;
    range->end = vaddr + size;
    range->flags = VMM_DEFAULT_KERNEL_FLAGS;
    range->is_anon = false;
    range->is_direct = true;
    range->paddr = paddr;
    range->next = address_space->ranges;
    address_space->ranges = range;
    return 0;
}

int vmm_alloc(vmm_address_space_t *address_space, uintptr_t vaddr, size_t size) {
    vmm_anon_t *prev = 0;
    for(size_t i = 0; i < size; i += ARCH_PAGE_SIZE) {
        vmm_anon_t *anon = heap_alloc(sizeof(vmm_anon_t));
        anon->page = pmm_alloc_page(PMM_AF_NORMAL);
        arch_vmm_map(address_space, vaddr + i, anon->page->paddr, VMM_DEFAULT_KERNEL_FLAGS);
        anon->offset = i;
        anon->next = prev;
        prev = anon;
    }

    vmm_range_t *range = heap_alloc(sizeof(vmm_range_t));
    range->start = vaddr;
    range->end = vaddr + size;
    range->flags = VMM_DEFAULT_KERNEL_FLAGS;
    range->is_anon = true;
    range->is_direct = false;
    range->anon = prev;
    range->next = address_space->ranges;
    address_space->ranges = range;
    return 0;
}

int vmm_free(vmm_address_space_t *address_space, uintptr_t vaddr, size_t size) {
    return -1;
}