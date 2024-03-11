#include <memory/vmm.h>
#include <lib/math.h>
#include <arch/vmm.h>
#include <arch/types.h>

#define FIXED_DATA(SEG) ((seg_fixed_data_t *) (SEG)->driver_data)
#define PADDR(SEG, ADDR) (FIXED_DATA(SEG)->phys_base + MATH_FLOOR((ADDR) - (uintptr_t) FIXED_DATA(SEG)->virt_base, ARCH_PAGE_SIZE))

static void seg_attach(vmm_segment_t *segment) {
    if(FIXED_DATA(segment)->virt_base != NULL) return;
    FIXED_DATA(segment)->virt_base = (void *) segment->base;
}

static void seg_detach(vmm_segment_t *segment) {
    for(uintptr_t length = 0; length < segment->length; length += ARCH_PAGE_SIZE) arch_vmm_unmap(segment->address_space, segment->base + length);
}

static bool seg_fault(vmm_segment_t *segment, uintptr_t address, int flags) {
    if((flags & VMM_FAULT_NONPRESENT) == 0) return false;
    int mapflags = ARCH_VMM_FLAG_NONE;
    if(segment->address_space != g_vmm_kernel_address_space) mapflags |= ARCH_VMM_FLAG_USER;
    arch_vmm_map(segment->address_space, MATH_FLOOR(address, ARCH_PAGE_SIZE), PADDR(segment, address), segment->protection, mapflags);
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