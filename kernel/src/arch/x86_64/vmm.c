#include <lib/container.h>
#include <lib/mem.h>
#include <common/assert.h>
#include <common/panic.h>
#include <memory/pmm.h>
#include <memory/hhdm.h>
#include <memory/heap.h>
#include <sys/ipl.h>
#include <sys/cpu.h>
#include <arch/vmm.h>
#include <arch/sched.h>
#include <arch/cpu.h>
#include <arch/types.h>
#include <arch/interrupt.h>
#include <arch/x86_64/init.h>
#include <arch/x86_64/interrupt.h>
#include <arch/x86_64/exception.h>
#include <arch/x86_64/sys/lapic.h>
#include <arch/x86_64/sys/cpu.h>

#define X86_64_AS(ADDRESS_SPACE) (CONTAINER_OF((ADDRESS_SPACE), x86_64_vmm_address_space_t, common))

#define VADDR_TO_INDEX(VADDR, LEVEL) (((VADDR) >> ((LEVEL) * 9 + 3)) & 0x1FF)
#define ADDRESS_MASK ((uint64_t) 0x000FFFFFFFFFF000)

#define KERNELSPACE_START 0xFFFF'8000'0000'0000
#define KERNELSPACE_END (UINT64_MAX - ARCH_PAGE_SIZE)
#define USERSPACE_START (ARCH_PAGE_SIZE)
#define USERSPACE_END (((uintptr_t) 1 << 47) - ARCH_PAGE_SIZE - 1)

typedef enum {
    PAGEFAULT_FLAG_PRESENT = (1 << 0),
    PAGEFAULT_FLAG_WRITE = (1 << 1),
    PAGEFAULT_FLAG_USER = (1 << 2),
    PAGEFAULT_FLAG_RESERVED_WRITE = (1 << 3),
    PAGEFAULT_FLAG_INSTRUCTION_FETCH = (1 << 4),
    PAGEFAULT_FLAG_PROTECTION_KEY = (1 << 5),
    PAGEFAULT_FLAG_SHADOW_STACK = (1 << 6),
    PAGEFAULT_FLAG_SGX = (1 << 7)
} pagefault_flag_t;

typedef enum {
    PTE_FLAG_PRESENT = (1 << 0),
    PTE_FLAG_RW = (1 << 1),
    PTE_FLAG_USER = (1 << 2),
    PTE_FLAG_WRITETHROUGH = (1 << 3),
    PTE_FLAG_DISABLECACHE = (1 << 4),
    PTE_FLAG_ACCESSED = (1 << 5),
    PTE_FLAG_PAT = (1 << 7),
    PTE_FLAG_GLOBAL = (1 << 8),
    PTE_FLAG_NX = ((uint64_t) 1 << 63)
} pte_flag_t;

typedef enum {
    PTE_PAT0 = 0,
    PTE_PAT1 = PTE_FLAG_WRITETHROUGH,
    PTE_PAT2 = PTE_FLAG_DISABLECACHE,
    PTE_PAT3 = PTE_FLAG_DISABLECACHE | PTE_FLAG_WRITETHROUGH,
    PTE_PAT4 = PTE_FLAG_PAT,
    PTE_PAT5 = PTE_FLAG_PAT | PTE_FLAG_WRITETHROUGH,
    PTE_PAT6 = PTE_FLAG_PAT | PTE_FLAG_DISABLECACHE,
    PTE_PAT7 = PTE_FLAG_PAT | PTE_FLAG_DISABLECACHE | PTE_FLAG_WRITETHROUGH
} pte_pat_t;

typedef struct {
    spinlock_t cr3_lock;
    uintptr_t cr3;
    vmm_address_space_t common;
} x86_64_vmm_address_space_t;

static uint8_t g_tlb_shootdown_vector;
static x86_64_vmm_address_space_t g_initial_address_space;

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

static uint64_t flags_cache_prot_to_x86_flags(vmm_protection_t prot, vmm_cache_t cache, int flags) {
    uint64_t x86_flags = 0;
    if((prot & VMM_PROT_READ) == 0) panic("!VMM_PROT_READ not supported");
    if((prot & VMM_PROT_WRITE) != 0) x86_flags |= PTE_FLAG_RW;
    if((prot & VMM_PROT_EXEC) == 0) x86_flags |= PTE_FLAG_NX;
    if((flags & ARCH_VMM_FLAG_USER) != 0) x86_flags |= PTE_FLAG_USER;
    if((flags & ARCH_VMM_FLAG_GLOBAL) != 0) x86_flags |= PTE_FLAG_GLOBAL;
    if(cache == VMM_CACHE_STANDARD) x86_flags |= PTE_PAT0;
    if(cache == VMM_CACHE_WRITE_COMBINE) x86_flags |= PTE_PAT4;
    return x86_flags;
}

static void tlb_shootdown(vmm_address_space_t *address_space) {
    if(x86_64_init_stage() < X86_64_INIT_STAGE_SCHED) {
        if(X86_64_AS(address_space) == &g_initial_address_space || read_cr3() == X86_64_AS(address_space)->cr3) write_cr3(read_cr3());
        return;
    }

    ipl_t old_ipl = ipl(IPL_CRITICAL);
    for(size_t i = 0; i < g_x86_64_cpu_count; i++) {
        x86_64_cpu_t *cpu = &g_x86_64_cpus[i];

        if(cpu == X86_64_CPU(cpu_current())) {
            if(X86_64_AS(address_space) == &g_initial_address_space || read_cr3() == X86_64_AS(address_space)->cr3) write_cr3(read_cr3());
            continue;
        }

        spinlock_acquire(&cpu->tlb_shootdown_lock);
        spinlock_acquire(&cpu->tlb_shootdown_check);
        cpu->tlb_shootdown_cr3 = X86_64_AS(address_space)->cr3;

        asm volatile("" : : : "memory");
        x86_64_lapic_ipi(cpu->lapic_id, g_tlb_shootdown_vector | X86_64_LAPIC_IPI_ASSERT);

        volatile int timeout = 0;
        do {
            if(timeout++ % 500 != 0) {
                asm volatile("pause");
                continue;
            }
            if(timeout >= 3000) break;
            x86_64_lapic_ipi(cpu->lapic_id, g_tlb_shootdown_vector | X86_64_LAPIC_IPI_ASSERT);
        } while(!spinlock_try_acquire(&cpu->tlb_shootdown_check));

        spinlock_release(&cpu->tlb_shootdown_check);
        spinlock_release(&cpu->tlb_shootdown_lock);
    }
    ipl(old_ipl);
}

static void tlb_shootdown_handler([[maybe_unused]] x86_64_interrupt_frame_t *frame) {
    x86_64_cpu_t *cpu = X86_64_CPU(cpu_current());
    if(spinlock_try_acquire(&cpu->tlb_shootdown_check)) return spinlock_release(&cpu->tlb_shootdown_check);
    if(cpu->tlb_shootdown_cr3 == g_initial_address_space.cr3 || read_cr3() == cpu->tlb_shootdown_cr3) write_cr3(read_cr3());
    spinlock_release(&cpu->tlb_shootdown_check);
}

vmm_address_space_t *arch_vmm_address_space_create() {
    x86_64_vmm_address_space_t *address_space = heap_alloc(sizeof(x86_64_vmm_address_space_t));
    address_space->cr3 = pmm_alloc_page(PMM_STANDARD | PMM_FLAG_ZERO)->paddr;
    memcpy((void *) HHDM(address_space->cr3 + 256 * sizeof(uint64_t)), (void *) HHDM(X86_64_AS(g_vmm_kernel_address_space)->cr3 + 256 * sizeof(uint64_t)), 256 * sizeof(uint64_t));

    address_space->cr3_lock = SPINLOCK_INIT;
    address_space->common.lock = SPINLOCK_INIT;
    address_space->common.segments = LIST_INIT;
    address_space->common.start = USERSPACE_START;
    address_space->common.end = USERSPACE_END;
    return &address_space->common;
}

vmm_address_space_t *x86_64_vmm_init() {
    g_initial_address_space.common.lock = SPINLOCK_INIT;
    g_initial_address_space.common.segments = LIST_INIT;
    g_initial_address_space.common.start = KERNELSPACE_START;
    g_initial_address_space.common.end = KERNELSPACE_END;
    g_initial_address_space.cr3 = pmm_alloc_page(PMM_STANDARD | PMM_FLAG_ZERO)->paddr;
    g_initial_address_space.cr3_lock = SPINLOCK_INIT;

    int vector = x86_64_interrupt_request(X86_64_INTERRUPT_PRIORITY_IPC, tlb_shootdown_handler);
    ASSERT(vector != -1);
    g_tlb_shootdown_vector = (uint8_t) vector;

    uint64_t *old_pml4 = (uint64_t *) HHDM(read_cr3());
    uint64_t *pml4 = (uint64_t *) HHDM(g_initial_address_space.cr3);
    for(int i = 256; i < 512; i++) {
        if(old_pml4[i] & PTE_FLAG_PRESENT) {
            pml4[i] = old_pml4[i];
            continue;
        }
        pmm_page_t *page = pmm_alloc_page(PMM_STANDARD | PMM_FLAG_ZERO);
        pml4[i] = PTE_FLAG_PRESENT | PTE_FLAG_RW; // Needs to be completely unrestricted as these are not synced across address spaces
        pte_set_address(&pml4[i], page->paddr);
    }

    return &g_initial_address_space.common;
}

void arch_vmm_load_address_space(vmm_address_space_t *address_space) {
    write_cr3(X86_64_AS(address_space)->cr3);
}

void arch_vmm_map(vmm_address_space_t *address_space, uintptr_t vaddr, uintptr_t paddr, vmm_protection_t prot, vmm_cache_t cache, int flags) {
    uint64_t x86_flags = flags_cache_prot_to_x86_flags(prot, cache, flags);
    spinlock_acquire(&X86_64_AS(address_space)->cr3_lock);
    uint64_t *current_table = (uint64_t *) HHDM(X86_64_AS(address_space)->cr3);
    for(int i = 4; i > 1; i--) {
        int index = VADDR_TO_INDEX(vaddr, i);
        if((current_table[index] & PTE_FLAG_PRESENT) != 0) {
            if((x86_flags & PTE_FLAG_NX) == 0) current_table[index] &= ~PTE_FLAG_NX;
        } else {
            pmm_page_t *page = pmm_alloc_page(PMM_STANDARD | PMM_FLAG_ZERO);
            current_table[index] = PTE_FLAG_PRESENT;
            pte_set_address(&current_table[index], page->paddr);
            if((x86_flags & PTE_FLAG_NX) != 0) current_table[index] |= PTE_FLAG_NX;
        }
        current_table[index] |= (x86_flags & (PTE_FLAG_RW | PTE_FLAG_USER));
        current_table = (uint64_t *) HHDM(pte_get_address(current_table[index]));
    }
    int index = VADDR_TO_INDEX(vaddr, 1);
    current_table[index] = PTE_FLAG_PRESENT | x86_flags;
    pte_set_address(&current_table[index], paddr);
    tlb_shootdown(address_space);
    spinlock_release(&X86_64_AS(address_space)->cr3_lock);
}

void arch_vmm_unmap(vmm_address_space_t *address_space, uintptr_t vaddr) {
    spinlock_acquire(&X86_64_AS(address_space)->cr3_lock);
    uint64_t *current_table = (uint64_t *) HHDM(X86_64_AS(address_space)->cr3);
    for(int i = 4; i > 1; i--) {
        int index = VADDR_TO_INDEX(vaddr, i);
        if(!(current_table[index] & PTE_FLAG_PRESENT)) {
            spinlock_release(&X86_64_AS(address_space)->cr3_lock);
            goto cleanup;
        }
        current_table = (uint64_t *) HHDM(pte_get_address(current_table[index]));
    }
    int index = VADDR_TO_INDEX(vaddr, 1);
    if(!(current_table[index] & PTE_FLAG_PRESENT)) goto cleanup;
    current_table[index] = 0;

    cleanup:
    tlb_shootdown(address_space);
    spinlock_release(&X86_64_AS(address_space)->cr3_lock);
}

bool arch_vmm_physical(vmm_address_space_t *address_space, uintptr_t vaddr, uintptr_t *out) {
    spinlock_acquire(&X86_64_AS(address_space)->cr3_lock);
    uint64_t *current_table = (uint64_t *) HHDM(X86_64_AS(address_space)->cr3);
    for(int i = 4; i > 1; i--) {
        int index = VADDR_TO_INDEX(vaddr, i);
        if(!(current_table[index] & PTE_FLAG_PRESENT)) {
            spinlock_release(&X86_64_AS(address_space)->cr3_lock);
            return false;
        }
        current_table = (uint64_t *) HHDM(pte_get_address(current_table[index]));
    }
    uint64_t entry = current_table[VADDR_TO_INDEX(vaddr, 1)];
    spinlock_release(&X86_64_AS(address_space)->cr3_lock);
    if(!(entry & PTE_FLAG_PRESENT)) return false;
    *out = pte_get_address(entry);
    return true;
}

void x86_64_vmm_page_fault_handler(x86_64_interrupt_frame_t *frame) {
    int flags = 0;
    if(!(frame->err_code & PAGEFAULT_FLAG_PRESENT)) flags |= VMM_FAULT_NONPRESENT;

    vmm_address_space_t *as = g_vmm_kernel_address_space;
    if(x86_64_init_stage() >= X86_64_INIT_STAGE_SCHED) {
        process_t *proc = arch_sched_thread_current()->proc;
        if(proc) as = proc->address_space;
    }

    uint64_t cr2;
    asm volatile("movq %%cr2, %0" : "=r" (cr2));
    if(vmm_fault(as, cr2, flags)) return;
    x86_64_exception_unhandled(frame);
}