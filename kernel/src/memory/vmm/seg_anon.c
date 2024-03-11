#include <memory/vmm.h>
#include <memory/pmm.h>
#include <lib/math.h>
#include <arch/vmm.h>
#include <arch/types.h>

static void map(vmm_segment_t *segment, uintptr_t address) {
    int flags = ARCH_VMM_FLAG_NONE;
    if(segment->address_space != g_vmm_kernel_address_space) flags |= ARCH_VMM_FLAG_USER;
    pmm_page_t *page = pmm_alloc_page(PMM_STANDARD);
    arch_vmm_map(segment->address_space, address, page->paddr, segment->protection, flags);
}

static void seg_attach(vmm_segment_t *segment) {
    if(segment->driver_data == NULL || !((bool) segment->driver_data)) return;
    for(uintptr_t length = 0; length < segment->length; length += ARCH_PAGE_SIZE) map(segment, segment->base + length);
}

static void seg_detach(vmm_segment_t *segment) {
    for(uintptr_t length = 0; length < segment->length; length += ARCH_PAGE_SIZE) arch_vmm_unmap(segment->address_space, segment->base + length);
}

static bool seg_fault(vmm_segment_t *segment, uintptr_t address, int flags) {
    if((flags & VMM_FAULT_NONPRESENT) == 0) return false;
    map(segment, MATH_FLOOR(address, ARCH_PAGE_SIZE));
    return true;
}

seg_driver_t g_seg_anon = (seg_driver_t) {
    .name = "anonymous",
    .ops = {
        .attach = seg_attach,
        .detach = seg_detach,
        .fault = seg_fault
    }
};