#include <arch/vmm.h>
#include <string.h>
#include <arch/types.h>
#include <memory/hhdm.h>
#include <memory/pmm.h>

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

void vmm_create_kernel_address_space(vmm_address_space_t *out) {
    out->ranges = 0;
    out->archdep.cr3 = read_cr3();
    memset((void *) HHDM(out->archdep.cr3), 0, 0x800);

    uint64_t *pml4 = (uint64_t *) HHDM(out->archdep.cr3);
    for(int i = 256; i < 512; i++) {
        if(pml4[i] & PTE_FLAG_PRESENT) continue;
        pmm_page_t *page = pmm_alloc_page();
        memset((void *) HHDM(page->paddr), 0, ARCH_PAGE_SIZE);
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
            pmm_page_t *page = pmm_alloc_page();
            memset((void *) HHDM(page->paddr), 0, ARCH_PAGE_SIZE);
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
    uint64_t *current_table = (uint64_t *) address_space->archdep.cr3;
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
    static uintptr_t highest_address;
    if(!highest_address) highest_address = ((uintptr_t) 1 << 48) - ARCH_PAGE_SIZE - 1;
    return highest_address;
}