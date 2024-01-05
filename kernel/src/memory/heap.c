#include "heap.h"
#include <lib/list.h>
#include <lib/spinlock.h>
#include <lib/assert.h>
#include <memory/pmm.h>
#include <arch/types.h>
#include <arch/vmm.h>

#define INITIAL_SIZE_PAGES 10
#define MIN_ENTRY_SIZE 8
#define HEAP_PROTECTION true

typedef struct {
#if HEAP_PROTECTION
    uint64_t prot;
#endif
    size_t size;
    bool free;
    list_element_t list_elem;
} heap_entry_t;

static vmm_address_space_t *g_as;
static uintptr_t g_base, g_limit, g_size;
static spinlock_t g_lock = SPINLOCK_INIT;
static list_t g_entries = LIST_INIT_CIRCULAR(g_entries);

#if HEAP_PROTECTION
static void update_prot(heap_entry_t *entry) {
    entry->prot = ~((uintptr_t) entry + entry->size);
}

static uint64_t get_prot(heap_entry_t *entry) {
    return (uint64_t) (uintptr_t) entry + entry->size + entry->prot + 1;
}
#endif

static void expand(size_t npages) {
    ASSERT(g_size + ARCH_PAGE_SIZE * npages <= g_limit);
    spinlock_acquire(&g_lock);
    for(size_t i = 0; i < npages; i++) {
        pmm_page_t *page = pmm_alloc_page(PMM_STANDARD);
        arch_vmm_map(g_as, g_base + g_size + i * ARCH_PAGE_SIZE, page->paddr, ARCH_VMM_FLAG_WRITE);
    }
    if(!list_is_empty(&g_entries) && LIST_GET(g_entries.prev, heap_entry_t, list_elem)->free) {
        heap_entry_t *entry = LIST_GET(g_entries.prev, heap_entry_t, list_elem);
        entry->size += npages * ARCH_PAGE_SIZE;
#if HEAP_PROTECTION
        update_prot(entry);
#endif
    } else {
        heap_entry_t *new = (heap_entry_t *) (g_base + g_size);
        new->free = true;
        new->size = (npages * ARCH_PAGE_SIZE) - sizeof(heap_entry_t);
#if HEAP_PROTECTION
        update_prot(new);
#endif
        list_prepend(&g_entries, &new->list_elem);
    }
    g_size += ARCH_PAGE_SIZE * npages;
    spinlock_release(&g_lock);
}

void heap_initialize(vmm_address_space_t *address_space, uintptr_t start, uintptr_t end) {
    g_as = address_space;
    g_base = start;
    g_limit = end;
    g_size = 0;
    expand(INITIAL_SIZE_PAGES);
}

void *heap_alloc_align(size_t size, size_t alignment) {
    ASSERT(size > 0);
    spinlock_acquire(&g_lock);
    list_element_t *elem;
    LIST_FOREACH(elem, &g_entries) {
        heap_entry_t *entry = LIST_GET(elem, heap_entry_t, list_elem);
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
        list_append(&entry->list_elem, &new->list_elem);
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
            list_append(&entry->list_elem, &overflow->list_elem);
            entry->size = size;
#if HEAP_PROTECTION
            update_prot(overflow);
            update_prot(entry);
#endif
        }

        spinlock_release(&g_lock);
        return (void *) ((uintptr_t) entry + sizeof(heap_entry_t));
    }
    spinlock_release(&g_lock);
    expand((size + sizeof(heap_entry_t)) / ARCH_PAGE_SIZE + 1);
    return heap_alloc_align(size, alignment);
}

void *heap_alloc(size_t size) {
    return heap_alloc_align(size, 1);
}

void heap_free(void* address) {
    ASSERT((uintptr_t) address >= g_base && (uintptr_t) address <= g_limit);
    spinlock_acquire(&g_lock);
    heap_entry_t *entry = (heap_entry_t *) (address - sizeof(heap_entry_t));
#if HEAP_PROTECTION
    ASSERT(get_prot(entry) == 0);
#endif
    entry->free = true;
    if(LIST_NEXT(&entry->list_elem) != &g_entries) {
        heap_entry_t *next = LIST_GET(LIST_NEXT(&entry->list_elem), heap_entry_t, list_elem);
        if(next->free) {
            entry->size += sizeof(heap_entry_t) + next->size;
            list_delete(&next->list_elem);
#if HEAP_PROTECTION
            update_prot(entry);
#endif
        }
    }
    if(LIST_PREVIOUS(&entry->list_elem) != &g_entries) {
        heap_entry_t *prev = LIST_GET(LIST_PREVIOUS(&entry->list_elem), heap_entry_t, list_elem);
        if(prev->free) {
            prev->size += sizeof(heap_entry_t) + entry->size;
            list_delete(&entry->list_elem);
#if HEAP_PROTECTION
            update_prot(prev);
#endif
        }
    }
    spinlock_release(&g_lock);
}