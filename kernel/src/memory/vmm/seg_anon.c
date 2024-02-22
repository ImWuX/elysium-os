#include <memory/vmm.h>
#include <memory/pmm.h>
#include <lib/math.h>
#include <common/kprint.h>
#include <arch/vmm.h>
#include <arch/types.h>

static void seg_attach([[maybe_unused]] vmm_segment_t *segment) {}

static void seg_detach(vmm_segment_t *segment) {
    for(uintptr_t length = 0; length < segment->length; length += ARCH_PAGE_SIZE) arch_vmm_unmap(segment->address_space, segment->base + length);
}

static bool seg_fault(vmm_segment_t *segment, uintptr_t address, int flags) {
    if(!(flags & VMM_FAULT_NONPRESENT)) return false;
    pmm_page_t *page = pmm_alloc_page(PMM_STANDARD);
    arch_vmm_map(segment->address_space, MATH_FLOOR(address, ARCH_PAGE_SIZE), page->paddr, segment->protection, ARCH_VMM_FLAG_NONE);
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