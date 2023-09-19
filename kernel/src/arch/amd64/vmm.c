#include <string.h>
#include <lib/list.h>
#include <lib/slock.h>
#include <arch/vmm.h>
#include <arch/types.h>
#include <memory/vmm.h>
#include <memory/pmm.h>
#include <memory/heap.h>
#include <memory/hhdm.h>

typedef enum {
    PTE_FLAG_PRESENT = 1,
    PTE_FLAG_RW = (1 << 1),
    PTE_FLAG_USER = (1 << 2),
    PTE_FLAG_WRITETHROUGH = (1 << 3),
    PTE_FLAG_DISABLECACHE = (1 << 4),
    PTE_FLAG_ACCESSED = (1 << 5),
    PTE_FLAG_PAT = (1 << 7),
    PTE_FLAG_NX = ((uint64_t) 1 << 63)
} vmm_pt_flag_t;

typedef enum {
    PAT_WRITE_BACK = 0,
    PAT_WRITE_THROUGH = PTE_FLAG_WRITETHROUGH,
    PAT_WEAK_UNCACHEABLE = PTE_FLAG_DISABLECACHE,
    PAT_STRONG_UNCACHEABLE = PTE_FLAG_DISABLECACHE | PTE_FLAG_WRITETHROUGH,
    PAT_WRITE_COMBINING = PTE_FLAG_PAT | PTE_FLAG_DISABLECACHE,
    PAT_WRITE_PROTECTED = PTE_FLAG_PAT | PTE_FLAG_WRITETHROUGH
} vmm_pt_pat_t;

static inline void pte_set_address(uint64_t *entry, uintptr_t address) {
    address &= 0x000FFFFFFFFFF000;
    *entry &= 0xFFF0000000000FFF;
    *entry |= address;
}

static inline uintptr_t pte_get_address(uint64_t entry) {
    return entry & 0x000FFFFFFFFFF000;
}

static inline void write_cr3(uint64_t value) {
    asm volatile("movq %0, %%cr3" : : "r" (value) : "memory");
}

static inline uint64_t read_cr3() {
    uint64_t value;
    asm volatile("movq %%cr3, %0" : "=r" (value));
    return value;
}

static uint64_t arch_independent_flags_to_x86(uint64_t flags) {
    uint64_t x86_flags = 0;
    if(flags & VMM_FLAGS_WRITE) x86_flags |= PTE_FLAG_RW;
    if(flags & VMM_FLAGS_USER) x86_flags |= PTE_FLAG_USER;
    if(!(flags & VMM_FLAGS_EXEC)) x86_flags |= PTE_FLAG_NX;
    return x86_flags;
}

vmm_address_space_t *vmm_fork(vmm_address_space_t *root) {
    pmm_page_t *pml4 = pmm_alloc_page(PMM_GENERAL | PMM_AF_ZERO);
    memcpy((void *) HHDM(pml4->paddr + 256 * sizeof(uint64_t)), (void *) HHDM(root->archdep.cr3 + 256 * sizeof(uint64_t)), 256 * sizeof(uint64_t));

    vmm_address_space_t *new = heap_alloc(sizeof(vmm_address_space_t));
    new->lock = SLOCK_INIT;
    new->segments = LIST_INIT;
    new->archdep.cr3 = pml4->paddr;
    return new;
}

void arch_vmm_init() {
    g_kernel_address_space.segments = LIST_INIT_CIRCULAR(g_kernel_address_space.segments);
    g_kernel_address_space.lock = SLOCK_INIT;
    g_kernel_address_space.archdep.cr3 = pmm_alloc_page(PMM_GENERAL | PMM_AF_ZERO)->paddr;

    uint64_t *old_pml4 = (uint64_t *) HHDM(read_cr3());
    uint64_t *pml4 = (uint64_t *) HHDM(g_kernel_address_space.archdep.cr3);
    for(int i = 256; i < 512; i++) {
        if(old_pml4[i] & PTE_FLAG_PRESENT) {
            pml4[i] = old_pml4[i];
            continue;
        }
        pmm_page_t *page = pmm_alloc_page(PMM_GENERAL | PMM_AF_ZERO);
        pml4[i] = PTE_FLAG_PRESENT | PTE_FLAG_NX;
        pte_set_address(&pml4[i], page->paddr);
    }
}

void arch_vmm_load_address_space(vmm_address_space_t *address_space) {
    write_cr3(address_space->archdep.cr3);
}

void arch_vmm_map(vmm_address_space_t *address_space, uintptr_t vaddr, uintptr_t paddr, uint64_t flags) {
    flags = arch_independent_flags_to_x86(flags);

    uint64_t *current_table = (uint64_t *) HHDM(address_space->archdep.cr3);
    for(int i = 4; i > 1; i--) {
        int index = (vaddr >> (i * 9 + 3)) & 0x1FF;
        if(current_table[index] & PTE_FLAG_PRESENT) {
            if(!(flags & PTE_FLAG_NX)) current_table[index] &= ~PTE_FLAG_NX;
        } else {
            pmm_page_t *page = pmm_alloc_page(PMM_GENERAL | PMM_AF_ZERO);
            current_table[index] = PTE_FLAG_PRESENT;
            pte_set_address(&current_table[index], page->paddr);
            if(flags & PTE_FLAG_NX) current_table[index] |= PTE_FLAG_NX;
        }
        if(flags & PTE_FLAG_RW) current_table[index] |= PTE_FLAG_RW;
        if(flags & PTE_FLAG_USER) current_table[index] |= PTE_FLAG_USER;
        current_table = (uint64_t *) HHDM(pte_get_address(current_table[index]));
    }

    int index = (vaddr >> 12) & 0x1FF;
    current_table[index] = PTE_FLAG_PRESENT;
    current_table[index] |= flags;
    pte_set_address(&current_table[index], paddr);
}

uintptr_t arch_vmm_physical(vmm_address_space_t *address_space, uintptr_t vaddr) {
    uint64_t *current_table = (uint64_t *) HHDM(address_space->archdep.cr3);
    for(uint8_t i = 4; i > 1; i--) {
        int index = (vaddr >> (i * 9 + 3)) & 0x1FF;
        uint64_t entry = current_table[index];
        if(!(current_table[index] & PTE_FLAG_PRESENT)) return 0;
        current_table = (uint64_t *) HHDM(pte_get_address(entry));
    }
    int index = (vaddr >> 12) & 0x1FF;
    uint64_t entry = current_table[index];
    if(!(current_table[index] & PTE_FLAG_PRESENT)) return 0;
    return pte_get_address(entry);
}

uintptr_t arch_vmm_highest_userspace_addr() {
    return ((uintptr_t) 1 << 47) - ARCH_PAGE_SIZE - 1;
}