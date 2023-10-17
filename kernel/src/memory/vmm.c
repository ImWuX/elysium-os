#include "vmm.h"
#include <errno.h>
#include <lib/assert.h>
#include <lib/round.h>
#include <memory/heap.h>
#include <memory/vmm/seg_anon.h>
#include <memory/vmm/seg_direct.h>
#include <arch/vmm.h>

#define VALIDATE_ARGS(VADDR, LENGTH) (VADDR % ARCH_PAGE_SIZE == 0 && length > 0 && length % ARCH_PAGE_SIZE == 0)

vmm_address_space_t *g_kernel_address_space;

static bool is_free(vmm_address_space_t *as, uintptr_t base, size_t length) {
    list_t *entry;
    LIST_FOREACH(entry, &as->segments) {
        vmm_segment_t *other = LIST_GET(entry, vmm_segment_t, list);
        if(other->base + (other->length - 1) >= base && other->base <= base + (length - 1)) return false;
    }
    return true;
}

int vmm_map(vmm_segment_t *segment) {
    slock_acquire(&segment->address_space->lock);
    if(!is_free(segment->address_space, segment->base, segment->length)) {
        segment->ops->free(segment);
        slock_release(&segment->address_space->lock);
        return -EEXIST;
    }
    int r = segment->ops->map(segment, segment->base, segment->length);
    if(r != 0) {
        segment->ops->free(segment);
        slock_release(&segment->address_space->lock);
        return r;
    }
    list_t *entry;
    LIST_FOREACH(entry, &segment->address_space->segments) {
        vmm_segment_t *other_seg = LIST_GET(entry, vmm_segment_t, list);
        if(other_seg->base > segment->base) break;
    }
    list_insert_behind(entry->prev, &segment->list);
    slock_release(&segment->address_space->lock);
    return 0;
}

int vmm_map_anon(vmm_address_space_t *as, uintptr_t vaddr, size_t length, int prot, bool wired) {
    if(!VALIDATE_ARGS(vaddr, length)) return -EINVAL;
    return vmm_map(seg_anon(as, vaddr, length, prot, wired));
}

int vmm_map_direct(vmm_address_space_t *as, uintptr_t vaddr, size_t length, int prot, uintptr_t paddr) {
    if(!VALIDATE_ARGS(vaddr, length)) return -EINVAL;
    return vmm_map(seg_direct(as, vaddr, length, prot, paddr));
}

int vmm_unmap(vmm_address_space_t *as, uintptr_t vaddr, size_t length) {
    if(!VALIDATE_ARGS(vaddr, length)) return -EINVAL;
    slock_acquire(&as->lock);

    panic("vmm_unmap is a no-op rn\n");

    slock_release(&as->lock);
    return 0;
}

bool vmm_fault(vmm_address_space_t *as, uintptr_t address) {
    list_t *entry;
    LIST_FOREACH(entry, &as->segments) {
        vmm_segment_t *segment = LIST_GET(entry, vmm_segment_t, list);
        if(segment->base > address) break;
        if(segment->base + segment->length - 1 < address) continue;
        return segment->ops->fault(segment, address);
    }
    return false;
}