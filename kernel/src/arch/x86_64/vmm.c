#include <arch/vmm.h>
#include <lib/container.h>
#include <memory/pmm.h>
#include <memory/hhdm.h>

#define ARCH_AS(ADDRESS_SPACE) (container_of(ADDRESS_SPACE, arch_vmm_address_space_t, common))

#define VADDR_TO_INDEX(VADDR, LEVEL) (((VADDR) >> ((LEVEL) * 9 + 3)) & 0x1FF)
#define ADDRESS_MASK ((uint64_t) 0x000FFFFFFFFFF000)

typedef enum {
    PTE_FLAG_PRESENT = 1,
    PTE_FLAG_RW = (1 << 1),
    PTE_FLAG_USER = (1 << 2),
    PTE_FLAG_WRITETHROUGH = (1 << 3),
    PTE_FLAG_DISABLECACHE = (1 << 4),
    PTE_FLAG_ACCESSED = (1 << 5),
    PTE_FLAG_PAT = (1 << 7),
    PTE_FLAG_GLOBAL = (1 << 8),
    PTE_FLAG_NX = ((uint64_t) 1 << 63)
} pte_flag_t;

typedef struct {
    uintptr_t cr3;
    vmm_address_space_t common;
} arch_vmm_address_space_t;

static arch_vmm_address_space_t g_initial_address_space;

static inline void pte_set_address(uint64_t *entry, uintptr_t address) {
    address &= ADDRESS_MASK;
    *entry &= ~ADDRESS_MASK;
    *entry |= address;
}

static inline uintptr_t pte_get_address(uint64_t entry) {
    return entry & ADDRESS_MASK;
}

static inline void write_cr3(uint64_t value) {
    asm volatile("movq %0, %%cr3" : : "r" (value) : "memory");
}

static inline uint64_t read_cr3() {
    uint64_t value;
    asm volatile("movq %%cr3, %0" : "=r" (value));
    return value;
}

static uint64_t flags_to_x86_flags(int flags) {
    uint64_t x86_flags = 0;
    if(flags & ARCH_VMM_FLAG_WRITE) x86_flags |= PTE_FLAG_RW;
    if(flags & ARCH_VMM_FLAG_USER) x86_flags |= PTE_FLAG_USER;
    if(flags & ARCH_VMM_FLAG_GLOBAL) x86_flags |= PTE_FLAG_GLOBAL;
    if(!(flags & ARCH_VMM_FLAG_EXEC)) x86_flags |= PTE_FLAG_NX;
    return x86_flags;
}

void arch_vmm_init() {
    g_initial_address_space.common.lock = SPINLOCK_INIT;
    g_initial_address_space.cr3 = pmm_alloc_page(PMM_STANDARD | PMM_AF_ZERO)->paddr;

    uint64_t *old_pml4 = (uint64_t *) HHDM(read_cr3());
    uint64_t *pml4 = (uint64_t *) HHDM(g_initial_address_space.cr3);
    for(int i = 256; i < 512; i++) {
        if(old_pml4[i] & PTE_FLAG_PRESENT) {
            pml4[i] = old_pml4[i];
            continue;
        }
        pmm_page_t *page = pmm_alloc_page(PMM_STANDARD | PMM_AF_ZERO);
        pml4[i] = PTE_FLAG_PRESENT | PTE_FLAG_NX;
        pte_set_address(&pml4[i], page->paddr);
    }

    g_vmm_kernel_address_space = &g_initial_address_space.common;
}

void arch_vmm_load_address_space(vmm_address_space_t *address_space) {
    write_cr3(ARCH_AS(address_space)->cr3);
}

void arch_vmm_map(vmm_address_space_t *address_space, uintptr_t vaddr, uintptr_t paddr, int flags) {
    uint64_t x86_flags = flags_to_x86_flags(flags);
    spinlock_acquire(&address_space->lock);
    uint64_t *current_table = (uint64_t *) HHDM(ARCH_AS(address_space)->cr3);
    for(int i = 4; i > 1; i--) {
        int index = VADDR_TO_INDEX(vaddr, i);
        if(current_table[index] & PTE_FLAG_PRESENT) {
            if(!(x86_flags & PTE_FLAG_NX)) current_table[index] &= ~PTE_FLAG_NX;
        } else {
            pmm_page_t *page = pmm_alloc_page(PMM_STANDARD | PMM_AF_ZERO);
            current_table[index] = PTE_FLAG_PRESENT;
            pte_set_address(&current_table[index], page->paddr);
            if(x86_flags & PTE_FLAG_NX) current_table[index] |= PTE_FLAG_NX;
        }
        if(x86_flags & PTE_FLAG_RW) current_table[index] |= PTE_FLAG_RW;
        if(x86_flags & PTE_FLAG_USER) current_table[index] |= PTE_FLAG_USER;
        current_table = (uint64_t *) HHDM(pte_get_address(current_table[index]));
    }

    int index = VADDR_TO_INDEX(vaddr, 1);
    current_table[index] = PTE_FLAG_PRESENT;
    current_table[index] |= x86_flags;
    pte_set_address(&current_table[index], paddr);
    spinlock_release(&address_space->lock);
}

bool arch_vmm_physical(vmm_address_space_t *address_space, uintptr_t vaddr, uintptr_t *out) {
    spinlock_acquire(&address_space->lock);
    uint64_t *current_table = (uint64_t *) HHDM(ARCH_AS(address_space)->cr3);
    for(int i = 4; i > 1; i--) {
        int index = VADDR_TO_INDEX(vaddr, i);
        if(!(current_table[index] & PTE_FLAG_PRESENT)) return false;
        current_table = (uint64_t *) HHDM(pte_get_address(current_table[index]));
    }
    uint64_t entry = current_table[VADDR_TO_INDEX(vaddr, 1)];
    spinlock_release(&address_space->lock);
    if(!(entry & PTE_FLAG_PRESENT)) return false;
    *out = pte_get_address(entry);
    return true;
}