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

static void segment_map(vmm_segment_t *segment, uintptr_t address, uintptr_t length) {
    ASSERT(address % ARCH_PAGE_SIZE == 0 && length % ARCH_PAGE_SIZE == 0);
    ASSERT(address < segment->base || address + length >= segment->base);

    int map_flags = ARCH_VMM_FLAG_NONE;
    if(segment->address_space != g_vmm_kernel_address_space) map_flags |= ARCH_VMM_FLAG_USER;

    for(size_t i = 0; i < length; i += ARCH_PAGE_SIZE) {
        uintptr_t virtual_address = address + i;
        uintptr_t physical_address = 0;
        switch(segment->type) {
            case VMM_SEGMENT_TYPE_ANON:
                pmm_flags_t physical_flags = PMM_STANDARD;
                if(segment->type_specific_data.anon.back_zeroed) physical_flags |= PMM_FLAG_ZERO;
                physical_address = pmm_alloc_page(physical_flags)->paddr;
                break;
            case VMM_SEGMENT_TYPE_DIRECT:
                physical_address = segment->type_specific_data.direct.physical_address + (virtual_address - segment->base);
                break;
        }
        arch_vmm_ptm_map(segment->address_space, virtual_address, physical_address, segment->protection, segment->cache, map_flags);
    }
}

static void segment_unmap(vmm_segment_t *segment, uintptr_t address, uintptr_t length) {
    ASSERT(address % ARCH_PAGE_SIZE == 0 && length % ARCH_PAGE_SIZE == 0);
    ASSERT(address < segment->base || address + length >= segment->base);

    switch(segment->type) {
        case VMM_SEGMENT_TYPE_ANON:
            // OPTIMIZE: invent page cache for segments
            // TODO: unmap phys mem
            break;
        case VMM_SEGMENT_TYPE_DIRECT: break;
    }
    for(uintptr_t i = 0; i < length; i += ARCH_PAGE_SIZE) arch_vmm_ptm_unmap(segment->address_space, address + i);
}

static vmm_segment_t *segments_alloc(bool kernel_as_lock_acquired) {
    spinlock_acquire(&g_segments_lock);
    if(list_is_empty(&g_segments_free)) {
        pmm_page_t *page = pmm_alloc_page(PMM_STANDARD);
        if(!kernel_as_lock_acquired) spinlock_acquire(&g_vmm_kernel_address_space->lock);
        uintptr_t address = find_space(g_vmm_kernel_address_space, 0, ARCH_PAGE_SIZE);
        arch_vmm_ptm_map(g_vmm_kernel_address_space, address, page->paddr, VMM_PROT_READ | VMM_PROT_WRITE, VMM_CACHE_STANDARD, ARCH_VMM_FLAG_NONE);

        vmm_segment_t *new_segments = (vmm_segment_t *) address;
        new_segments[0].address_space = g_vmm_kernel_address_space;
        new_segments[0].base = address;
        new_segments[0].length = ARCH_PAGE_SIZE;
        new_segments[0].protection = VMM_PROT_READ | VMM_PROT_WRITE;
        new_segments[0].type = VMM_SEGMENT_TYPE_ANON;
        new_segments[0].cache = VMM_CACHE_STANDARD;

        list_append(&g_vmm_kernel_address_space->segments, &new_segments[0].list_elem);

        for(unsigned int i = 1; i < ARCH_PAGE_SIZE / sizeof(vmm_segment_t); i++) list_append(&g_segments_free, &new_segments[i].list_elem);

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

// OPTIMIZE: this is very slow/inefficient, segments should probably be ordered or in a tree and we could do this fast
static bool memory_exists(vmm_address_space_t *address_space, uintptr_t address, size_t length) {
    if(!ADDRESS_IN_BOUNDS(address_space, address) || !ADDRESS_IN_BOUNDS(address_space, address + length)) return false;
    restart:
    if(length == 0) return true;
    LIST_FOREACH(&address_space->segments, elem) {
        vmm_segment_t *segment = LIST_CONTAINER_GET(elem, vmm_segment_t, list_elem);
        if(segment->base <= address && segment->base + segment->length > address) {
            uintptr_t new_addr = segment->base + segment->length;
            if(new_addr >= address + length) return true;
            length = (address + length) - new_addr;
            address = new_addr;
            goto restart;
        }
        if(segment->base > address && segment->base < address + length && segment->base + segment->length >= address + length) {
            length -= (address + length) - segment->base;
            goto restart;
        }
    }
    return false;
}

static void *map_common(
    vmm_address_space_t *address_space,
    void *hint,
    size_t length,
    vmm_protection_t prot,
    vmm_cache_t cache,
    vmm_flags_t flags,
    vmm_segment_type_t type,
    uintptr_t direct_physical_address
) {
    log(LOG_LEVEL_DEBUG, "VMM", "map(hint: %#lx, length: %#lx, prot: %c%c%c, flags: %lu, cache: %u, type: %u)",
        (uintptr_t) hint,
        length,
        prot & VMM_PROT_READ ? 'R' : '-',
        prot & VMM_PROT_WRITE ? 'W' : '-',
        prot & VMM_PROT_EXEC ? 'E' : '-',
        flags,
        cache,
        type
    );

    uintptr_t address = (uintptr_t) hint;
    if(length == 0 || length % ARCH_PAGE_SIZE != 0) return NULL;
    if(address % ARCH_PAGE_SIZE != 0) {
        if(flags & VMM_FLAG_FIXED) return NULL;
        address += ARCH_PAGE_SIZE - (address % ARCH_PAGE_SIZE);
    }

    vmm_segment_t *segment = segments_alloc(false);
    spinlock_acquire(&address_space->lock);
    address = find_space(address_space, address, length);
    if(address == 0 || ((uintptr_t) hint != address && (flags & VMM_FLAG_FIXED) != 0)) {
        segments_free(segment, false);
        spinlock_release(&address_space->lock);
        return NULL;
    }

    ASSERT(SEGMENT_IN_BOUNDS(address_space, address, length));
    ASSERT(address % ARCH_PAGE_SIZE == 0 && length % ARCH_PAGE_SIZE == 0);

    segment->address_space = address_space;
    segment->base = address;
    segment->length = length;
    segment->type = type;
    segment->protection = prot;
    segment->cache = cache;

    switch(segment->type) {
        case VMM_SEGMENT_TYPE_ANON: segment->type_specific_data.anon.back_zeroed = (flags & VMM_FLAG_ANON_ZERO) != 0; break;
        case VMM_SEGMENT_TYPE_DIRECT: segment->type_specific_data.direct.physical_address = direct_physical_address; break;
    }

    if((flags & VMM_FLAG_NO_DEMAND) != 0) segment_map(segment, segment->base, segment->length);

    list_append(&address_space->segments, &segment->list_elem);
    spinlock_release(&address_space->lock);

    log(LOG_LEVEL_DEBUG, "VMM", "map success (base: %#lx, length: %#lx)", segment->base, segment->length);
    return (void *) segment->base;
}

void *vmm_map_anon(vmm_address_space_t *address_space, void *hint, size_t length, vmm_protection_t prot, vmm_cache_t cache, vmm_flags_t flags) {
    return map_common(address_space, hint, length, prot, cache, flags, VMM_SEGMENT_TYPE_ANON, 0);
}

void *vmm_map_direct(vmm_address_space_t *address_space, void *hint, size_t length, vmm_protection_t prot, vmm_cache_t cache, vmm_flags_t flags, uintptr_t physical_address) {
    return map_common(address_space, hint, length, prot, cache, flags, VMM_SEGMENT_TYPE_DIRECT, physical_address);
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

        segment_unmap(split_segment, split_base, split_length);
        if(split_segment->base + split_segment->length > split_base + split_length) {
            vmm_segment_t *segment = segments_alloc(address_space == g_vmm_kernel_address_space);
            segment->address_space = address_space;
            segment->base = split_base + split_length;
            segment->length = (split_segment->base + split_segment->length) - (split_base + split_length);
            segment->protection = split_segment->protection;
            segment->type = split_segment->type;
            switch(segment->type) {
                case VMM_SEGMENT_TYPE_ANON: break;
                case VMM_SEGMENT_TYPE_DIRECT: segment->type_specific_data = split_segment->type_specific_data; break;
            }

            list_append(&split_segment->list_elem, &segment->list_elem);
        }

        if(split_segment->base < split_base) {
            split_segment->length = split_base - split_segment->base;
        } else {
            list_delete(&split_segment->list_elem);
            segments_free(split_segment, address_space == g_vmm_kernel_address_space);
        }
    }
    spinlock_release(&address_space->lock);
}

bool vmm_fault(vmm_address_space_t *address_space, uintptr_t address, int flags) {
    if((flags & VMM_FAULT_NONPRESENT) == 0) return false;

    vmm_segment_t *segment = NULL;
    if(ADDRESS_IN_BOUNDS(g_vmm_kernel_address_space, address)) {
        segment = addr_to_segment(g_vmm_kernel_address_space, address);
    } else {
        segment = addr_to_segment(address_space, address);
    }
    if(segment == NULL) return false;

    segment_map(segment, MATH_FLOOR(address, ARCH_PAGE_SIZE), ARCH_PAGE_SIZE);
    return true;
}

size_t vmm_copy_to(vmm_address_space_t *dest_as, uintptr_t dest_addr, void *src, size_t count) {
    if(!memory_exists(dest_as, dest_addr, count)) return 0;
    size_t i = 0;
    while(i < count) {
        size_t offset = (dest_addr + i) % ARCH_PAGE_SIZE;
        uintptr_t phys;
        if(!arch_vmm_ptm_physical(dest_as, dest_addr + i, &phys)) {
            if(!vmm_fault(dest_as, dest_addr + i, VMM_FAULT_NONPRESENT)) return i;
            ASSERT(arch_vmm_ptm_physical(dest_as, dest_addr + i, &phys));
        }

        size_t len = math_min(count - i, ARCH_PAGE_SIZE - offset);
        memcpy((void *) HHDM(phys + offset), src, len);
        i += len;
        src += len;
    }
    return i;
}

size_t vmm_copy_from(void *dest, vmm_address_space_t *src_as, uintptr_t src_addr, size_t count) {
    if(!memory_exists(src_as, src_addr, count)) return 0;
    size_t i = 0;
    while(i < count) {
        size_t offset = (src_addr + i) % ARCH_PAGE_SIZE;
        uintptr_t phys;
        if(!arch_vmm_ptm_physical(src_as, src_addr + i, &phys)) {
            if(!vmm_fault(src_as, src_addr + i, VMM_FAULT_NONPRESENT)) return i;
            ASSERT(arch_vmm_ptm_physical(src_as, src_addr + i, &phys));
        }

        size_t len = math_min(count - i, ARCH_PAGE_SIZE - offset);
        memcpy(dest, (void *) HHDM(phys + offset), len);
        i += len;
        dest += len;
    }
    return i;
}