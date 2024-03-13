#include <memory/vmm.h>
#include <memory/pmm.h>
#include <lib/math.h>
#include <arch/vmm.h>
#include <arch/types.h>

#define HAS_FLAG(SEG, FLAG) ((SEG)->driver_data != NULL && (((uint64_t) (SEG)->driver_data) & (FLAG)) != 0)

static void map(vmm_segment_t *segment, uintptr_t address) {
    int pmm_flags = PMM_STANDARD;
    int vmm_flags = ARCH_VMM_FLAG_NONE;
    if(segment->address_space != g_vmm_kernel_address_space) vmm_flags |= ARCH_VMM_FLAG_USER;
    if(HAS_FLAG(segment, SEG_ANON_FLAG_ZERO)) vmm_flags |= PMM_FLAG_ZERO;
    pmm_page_t *page = pmm_alloc_page(pmm_flags);
    arch_vmm_map(segment->address_space, address, page->paddr, segment->protection, vmm_flags);
}

static void seg_attach(vmm_segment_t *segment) {
    if(!HAS_FLAG(segment, SEG_ANON_FLAG_PREALLOC)) return;
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