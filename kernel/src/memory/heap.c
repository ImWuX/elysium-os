#include "heap.h"
#include <lib/list.h>
#include <lib/slock.h>
#include <lib/assert.h>
#include <arch/vmm.h>

#define MIN_ENTRY_SIZE 8
#define HEAP_PROTECTION true

typedef struct {
#if HEAP_PROTECTION
    uint64_t prot;
#endif
    size_t size;
    bool free;
    list_t list;
} heap_entry_t;

static slock_t g_lock = SLOCK_INIT;
static list_t g_entries = LIST_INIT_CIRCULAR(g_entries);

static vmm_segment_t g_heap_segment;

int heap_seg_map(vmm_segment_t *segment [[maybe_unused]], uintptr_t base [[maybe_unused]], size_t length [[maybe_unused]]) { return 0; }

int heap_seg_unmap(vmm_segment_t *segment [[maybe_unused]], uintptr_t base[[maybe_unused]] , size_t length[[maybe_unused]] ) { ASSERT_UNREACHABLE(); }

static bool heap_seg_fault(vmm_segment_t *segment, uintptr_t address) {
    arch_vmm_map(segment->address_space, address & ~0xFFF, pmm_alloc_page(PMM_GENERAL | PMM_AF_ZERO)->paddr, segment->protection);
    return true;
}

void heap_seg_free(vmm_segment_t *segment [[maybe_unused]]) { ASSERT_UNREACHABLE(); }

static vmm_segment_ops_t g_ops = {
    .map = &heap_seg_map,
    .unmap = &heap_seg_unmap,
    .fault = &heap_seg_fault,
    .free = &heap_seg_free
};

#if HEAP_PROTECTION
static void update_prot(heap_entry_t *entry) {
    entry->prot = ~((uintptr_t) entry + entry->size);
}

static uint64_t get_prot(heap_entry_t *entry) {
    return (uint64_t) (uintptr_t) entry + entry->size + entry->prot + 1;
}
#endif

void heap_initialize(vmm_address_space_t *address_space, uintptr_t start, uintptr_t end) {
    g_heap_segment = (vmm_segment_t) {
        .address_space = address_space,
        .base = start,
        .length = end - start,
        .protection = VMM_PROT_WRITE,
        .ops = &g_ops
    };
    ASSERT(vmm_map(&g_heap_segment) == 0);

    heap_entry_t *entry = (heap_entry_t *) g_heap_segment.base;
    entry->free = true;
    entry->size = g_heap_segment.length - sizeof(heap_entry_t);
#if HEAP_PROTECTION
    update_prot(entry);
#endif
    list_insert_behind(&g_entries, &entry->list);
}

void *heap_alloc_align(size_t size, size_t alignment) {
    ASSERT(size > 0);
    slock_acquire(&g_lock);
    list_t *lentry;
    LIST_FOREACH(lentry, &g_entries) {
        heap_entry_t *entry = LIST_GET(lentry, heap_entry_t, list);
        if(!entry->free) continue;
#if HEAP_PROTECTION
    ASSERT(get_prot(entry) == 0);
#endif

        uintptr_t mod = ((uintptr_t) entry + sizeof(heap_entry_t)) % alignment;
        uintptr_t offset = alignment - mod;
        if(mod == 0) offset = 0;

        check_fit:
        if(entry->size < offset + size) continue;
        if(offset == 0) goto aligned;
        if(offset < sizeof(heap_entry_t) + MIN_ENTRY_SIZE) {
            offset += alignment;
            goto check_fit;
        }

        heap_entry_t *new = (heap_entry_t *) ((uintptr_t) entry + offset);
        new->free = true;
        new->size = entry->size - offset;
        list_insert_behind(&entry->list, &new->list);
        entry->size = offset - sizeof(heap_entry_t);
#if HEAP_PROTECTION
        update_prot(new);
        update_prot(entry);
#endif
        entry = new;

        aligned:
        entry->free = false;
        if(entry->size - size > sizeof(heap_entry_t) + MIN_ENTRY_SIZE) {
            heap_entry_t *overflow = (heap_entry_t *) ((uintptr_t) entry + sizeof(heap_entry_t) + size);
            overflow->free = true;
            overflow->size = entry->size - size - sizeof(heap_entry_t);
            list_insert_behind(&entry->list, &overflow->list);
            entry->size = size;
#if HEAP_PROTECTION
            update_prot(overflow);
            update_prot(entry);
#endif
        }

        slock_release(&g_lock);
        return (void *) ((uintptr_t) entry + sizeof(heap_entry_t));
    }
    slock_release(&g_lock);
    ASSERT_UNREACHABLE();
}

void *heap_alloc(size_t size) {
    return heap_alloc_align(size, 1);
}

void heap_free(void* address) {
    ASSERT((uintptr_t) address >= g_heap_segment.base && (uintptr_t) address <= g_heap_segment.base + g_heap_segment.length - 1);
    slock_acquire(&g_lock);
    heap_entry_t *entry = (heap_entry_t *) (address - sizeof(heap_entry_t));
#if HEAP_PROTECTION
    ASSERT(get_prot(entry) == 0);
#endif
    entry->free = true;
    if(entry->list.next != &g_entries) {
        heap_entry_t *next = LIST_GET(entry->list.next, heap_entry_t, list);
        if(next->free) {
            entry->size += sizeof(heap_entry_t) + next->size;
            list_delete(&next->list);
#if HEAP_PROTECTION
            update_prot(entry);
#endif
        }
    }
    if(entry->list.prev != &g_entries) {
        heap_entry_t *prev = LIST_GET(entry->list.prev, heap_entry_t, list);
        if(prev->free) {
            prev->size += sizeof(heap_entry_t) + entry->size;
            list_delete(&entry->list);
#if HEAP_PROTECTION
            update_prot(prev);
#endif
        }
    }
    slock_release(&g_lock);
}