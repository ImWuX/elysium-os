#include "vmm.h"
#include <lib/mem.h>
#include <lib/math.h>
#include <common/log.h>
#include <common/assert.h>
#include <memory/pmm.h>
#include <memory/hhdm.h>
#include <arch/vmm.h>
#include <arch/types.h>

#define ADDRESS_IN_BOUNDS(ADDRESS_SPACE, ADDRESS) ((ADDRESS) >= (ADDRESS_SPACE)->start && (ADDRESS) < (ADDRESS_SPACE)->end)
#define SEGMENT_IN_BOUNDS(ADDRESS_SPACE, BASE, LENGTH) (ADDRESS_IN_BOUNDS((ADDRESS_SPACE), (BASE)) && ((ADDRESS_SPACE)->end - (BASE)) >= (LENGTH))

#define ADDRESS_IN_SEGMENT(ADDRESS, BASE, LENGTH) ((ADDRESS) >= (BASE) && (ADDRESS) < ((BASE) + (LENGTH)))
#define SEGMENT_INTERSECTS(BASE1, LENGTH1, BASE2, LENGTH2) ((BASE1) < ((BASE2) + (LENGTH2)) && (BASE2) < ((BASE1) + (LENGTH1)))

vmm_address_space_t *g_vmm_kernel_address_space;

static spinlock_t g_segments_lock = SPINLOCK_INIT;
static list_t g_segments_free = LIST_INIT;

static_assert(ARCH_PAGE_SIZE > (sizeof(vmm_segment_t) * 2));

/** @warning Assumes lock is acquired */
static uintptr_t find_space(vmm_address_space_t *address_space, uintptr_t address, size_t length) {
    if(!SEGMENT_IN_BOUNDS(address_space, address, length)) address = address_space->start;
    while(true) {
        if(!SEGMENT_IN_BOUNDS(address_space, address, length)) return 0;
        bool valid = true;
        LIST_FOREACH(&address_space->segments, elem) {
            vmm_segment_t *segment = LIST_CONTAINER_GET(elem, vmm_segment_t, list_elem);
            if(!SEGMENT_INTERSECTS(address, length, segment->base, segment->length)) continue;
            valid = false;
            address = segment->base + segment->length;
            break;
        }
        if(valid) break;
    }
    return address;
}

static vmm_segment_t *segments_alloc(bool kernel_as_lock_acquired) {
    spinlock_acquire(&g_segments_lock);
    if(list_is_empty(&g_segments_free)) {
        pmm_page_t *page = pmm_alloc_page(PMM_STANDARD);
        if(!kernel_as_lock_acquired) spinlock_acquire(&g_vmm_kernel_address_space->lock);
        uintptr_t address = find_space(g_vmm_kernel_address_space, 0, ARCH_PAGE_SIZE);
        arch_vmm_map(g_vmm_kernel_address_space, address, page->paddr, VMM_PROT_READ | VMM_PROT_WRITE, ARCH_VMM_FLAG_NONE);

        vmm_segment_t *new_segments = (vmm_segment_t *) address;
        new_segments[0].address_space = g_vmm_kernel_address_space;
        new_segments[0].base = address;
        new_segments[0].length = ARCH_PAGE_SIZE;
        new_segments[0].protection = VMM_PROT_READ | VMM_PROT_WRITE;
        new_segments[0].driver = &g_seg_anon;
        new_segments[0].driver_data = NULL;
        new_segments[0].driver->ops.attach(&new_segments[0]);
        list_append(&g_vmm_kernel_address_space->segments, &new_segments[0].list_elem);

        for(unsigned int i = 1; i < ARCH_PAGE_SIZE / sizeof(vmm_segment_t); i++) {
            list_append(&g_segments_free, &new_segments[i].list_elem);
        }

        if(!kernel_as_lock_acquired) spinlock_release(&g_vmm_kernel_address_space->lock);
    }
    list_element_t *elem = LIST_NEXT(&g_segments_free);
    ASSERT(elem != NULL);
    list_delete(elem);
    spinlock_release(&g_segments_lock);
    return LIST_CONTAINER_GET(elem, vmm_segment_t, list_elem);
}

static void segments_free(vmm_segment_t *segment, bool kernel_as_lock_acquired) {
    if(!kernel_as_lock_acquired) spinlock_acquire(&g_segments_lock);
    list_append(&g_segments_free, &segment->list_elem);
    if(!kernel_as_lock_acquired) spinlock_release(&g_segments_lock);
}

static vmm_segment_t *addr_to_segment(vmm_address_space_t *address_space, uintptr_t address) {
    if(!ADDRESS_IN_BOUNDS(address_space, address)) return NULL;
    LIST_FOREACH(&address_space->segments, elem) {
        vmm_segment_t *segment = LIST_CONTAINER_GET(elem, vmm_segment_t, list_elem);
        if(!ADDRESS_IN_SEGMENT(address, segment->base, segment->length)) continue;
        return segment;
    }
    return NULL;
}

void *vmm_map(vmm_address_space_t *address_space, void *address, size_t length, vmm_protection_t prot, vmm_flags_t flags, seg_driver_t *driver, void *driver_data) {
    log(LOG_LEVEL_DEBUG, "VMM", "map(address: %#lx, length: %#lx, prot: %c%c%c, flags: %lu, driver: %s)",
        (uintptr_t) address,
        length,
        prot & VMM_PROT_READ ? 'R' : '-',
        prot & VMM_PROT_WRITE ? 'W' : '-',
        prot & VMM_PROT_EXEC ? 'E' : '-',
        flags,
        driver->name
    );
    uintptr_t caddr = (uintptr_t) address;
    if(length == 0 || length % ARCH_PAGE_SIZE != 0) return NULL;
    if(caddr % ARCH_PAGE_SIZE != 0) {
        if(flags & VMM_FLAG_FIXED) return NULL;
        caddr += ARCH_PAGE_SIZE - (caddr % ARCH_PAGE_SIZE);
    }

    vmm_segment_t *segment = segments_alloc(false);
    spinlock_acquire(&address_space->lock);
    caddr = find_space(address_space, caddr, length);
    if(caddr == 0 || ((uintptr_t) address != caddr && (flags & VMM_FLAG_FIXED))) {
        spinlock_release(&address_space->lock);
        segments_free(segment, false);
        return NULL;
    }

    ASSERT(SEGMENT_IN_BOUNDS(address_space, caddr, length));
    ASSERT(caddr % ARCH_PAGE_SIZE == 0 && length % ARCH_PAGE_SIZE == 0);

    segment->address_space = address_space;
    segment->base = caddr;
    segment->length = length;
    segment->protection = prot;
    segment->driver = driver;
    segment->driver_data = driver_data;
    segment->driver->ops.attach(segment);
    list_append(&address_space->segments, &segment->list_elem);

    spinlock_release(&address_space->lock);
    return (void *) segment->base;
}

void vmm_unmap(vmm_address_space_t *address_space, void *address, size_t length) {
    log(LOG_LEVEL_DEBUG, "VMM", "unmap(address: %#lx, length: %#lx)", (uintptr_t) address, length);
    if(length == 0) return;
    ASSERT((uintptr_t) address % ARCH_PAGE_SIZE == 0 && length % ARCH_PAGE_SIZE == 0);
    ASSERT(SEGMENT_IN_BOUNDS(address_space, (uintptr_t) address, length));

    spinlock_acquire(&address_space->lock);
    for(uintptr_t split_base = (uintptr_t) address, split_length = 0; split_base < (uintptr_t) address + length; split_base += split_length) {
        split_length = ARCH_PAGE_SIZE;
        vmm_segment_t *split_segment = addr_to_segment(address_space, split_base);
        if(!split_segment) continue;

        while(
            ADDRESS_IN_SEGMENT(split_base + split_length, split_segment->base, split_segment->length) &&
            ADDRESS_IN_SEGMENT(split_base + split_length, (uintptr_t) address, length)
        ) split_length += ARCH_PAGE_SIZE;

        ASSERT(SEGMENT_IN_BOUNDS(address_space, split_base, split_length));
        ASSERT(split_base % ARCH_PAGE_SIZE == 0 && split_length % ARCH_PAGE_SIZE == 0);

        split_segment->driver->ops.detach(split_segment);
        if(split_segment->base + split_segment->length > split_base + split_length) {
            vmm_segment_t *segment = segments_alloc(address_space == g_vmm_kernel_address_space);
            segment->address_space = address_space;
            segment->base = split_base + split_length;
            segment->length = (split_segment->base + split_segment->length) - (split_base + split_length);
            segment->protection = split_segment->protection;
            segment->driver = split_segment->driver;
            segment->driver_data = split_segment->driver_data;
            segment->driver->ops.attach(segment);
            list_append(&split_segment->list_elem, &segment->list_elem);
        }

        if(split_segment->base < split_base) {
            split_segment->length = split_base - split_segment->base;
            split_segment->driver->ops.attach(split_segment);
        } else {
            list_delete(&split_segment->list_elem);
            segments_free(split_segment, address_space == g_vmm_kernel_address_space);
        }
    }
    spinlock_release(&address_space->lock);
}

bool vmm_fault(vmm_address_space_t *address_space, uintptr_t address, int flags) {
    vmm_segment_t *segment = NULL;
    if(ADDRESS_IN_BOUNDS(g_vmm_kernel_address_space, address)) {
        segment = addr_to_segment(g_vmm_kernel_address_space, address);
    } else {
        segment = addr_to_segment(address_space, address);
    }
    if(!segment) return false;
    return segment->driver->ops.fault(segment, address, flags);
}

size_t vmm_copy_to(vmm_address_space_t *dest_as, uintptr_t dest_addr, void *src, size_t count) {
    size_t offset = dest_addr % ARCH_PAGE_SIZE;
    size_t i = 0;
    while(i < count) {
        uintptr_t phys;
        if(!arch_vmm_physical(dest_as, dest_addr + i, &phys)) {
            if(!vmm_fault(dest_as, dest_addr + i, VMM_FAULT_NONPRESENT)) return i;
            ASSERT(arch_vmm_physical(dest_as, dest_addr + i, &phys));
        }

        size_t len = math_min(count - i, offset != 0 ? ARCH_PAGE_SIZE - offset : ARCH_PAGE_SIZE);
        memcpy((void *) HHDM(phys + offset), src, len);
        i += len;
        src += len;
        offset = 0;
    }
    return i;
}

size_t vmm_copy_from(void *dest, vmm_address_space_t *src_as, uintptr_t src_addr, size_t count) {
    size_t offset = src_addr % ARCH_PAGE_SIZE;
    size_t i = 0;
    while(i < count) {
        uintptr_t phys;
        if(!arch_vmm_physical(src_as, src_addr + i, &phys)) return i;

        size_t len = math_min(count - i, offset != 0 ? ARCH_PAGE_SIZE - offset : ARCH_PAGE_SIZE);
        memcpy(dest, (void *) HHDM(phys + offset), len);
        i += len;
        dest += len;
        offset = 0;
    }
    return i;
}