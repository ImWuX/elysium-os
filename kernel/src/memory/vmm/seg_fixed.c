#include <memory/vmm.h>
#include <lib/math.h>
#include <arch/vmm.h>
#include <arch/types.h>

static void seg_attach(vmm_segment_t *segment) {
    seg_fixed_data_t *data = (seg_fixed_data_t *) segment->driver_data;
    if(data->base != NULL) return;
    data->base = (void *) segment->base;
}

static void seg_detach(vmm_segment_t *segment) {
    for(uintptr_t length = 0; length < segment->length; length += ARCH_PAGE_SIZE) arch_vmm_unmap(segment->address_space, segment->base + length);
}

static bool seg_fault(vmm_segment_t *segment, uintptr_t address, int flags) {
    if(!(flags & VMM_FAULT_NONPRESENT)) return false;

    seg_fixed_data_t *data = (seg_fixed_data_t *) segment->driver_data;
    uintptr_t paddr = data->phys + MATH_FLOOR(address - (uintptr_t) data->base, ARCH_PAGE_SIZE);
    arch_vmm_map(segment->address_space, MATH_FLOOR(address, ARCH_PAGE_SIZE), paddr, segment->protection, segment->address_space == g_vmm_kernel_address_space ? ARCH_VMM_FLAG_NONE : ARCH_VMM_FLAG_USER);

    return true;
}

seg_driver_t g_seg_fixed = (seg_driver_t) {
    .name = "fixed",
    .ops = {
        .attach = seg_attach,
        .detach = seg_detach,
        .fault = seg_fault
    }
};