#include <tartarus.h>
#include <memory/hhdm.h>
#include <memory/pmm.h>
#include <memory/heap.h>
#include <lib/kprint.h>
#include <lib/panic.h>
#include <lib/mem.h>
#include <lib/list.h>
#include <lib/assert.h>
#include <lib/round.h>
#include <arch/cpu.h>
#include <arch/types.h>
#include <arch/vmm.h>
#include <arch/interrupt.h>
#include <arch/x86_64/cpu.h>
#include <arch/x86_64/vmm.h>
#include <arch/x86_64/lapic.h>
#include <arch/x86_64/port.h>
#include <arch/x86_64/msr.h>
#include <arch/x86_64/gdt.h>
#include <arch/x86_64/interrupt.h>
#include <arch/x86_64/exception.h>
#include <arch/x86_64/dev/pic8259.h>

#define ADJUST_STACK(OFFSET) asm volatile("mov %%rsp, %%rax\nadd %0, %%rax\nmov %%rax, %%rsp\nmov %%rbp, %%rax\nadd %0, %%rax\nmov %%rax, %%rbp" : : "rm" (OFFSET) : "rax", "memory")

uintptr_t g_hhdm_base;
size_t g_hhdm_size;

static vmm_segment_t g_hhdm_segment;
static vmm_segment_t g_kernel_segment;

x86_64_cpu_t g_x86_64_cpus[256] = {};
volatile size_t g_x86_64_cpus_initialized = 0;

static void pch(char ch) {
    x86_64_port_outb(0x3F8, ch);
}

static void init_common() {
    x86_64_cpu_t *cpu = &g_x86_64_cpus[g_x86_64_cpus_initialized];
    memset(cpu, 0, sizeof(x86_64_cpu_t));
    cpu->this = cpu;
    cpu->lapic_id = x86_64_lapic_id();
    cpu->tlb_shootdown_check = SPINLOCK_INIT;
    cpu->tlb_shootdown_lock = SPINLOCK_INIT;
    cpu->common.address_space = g_vmm_kernel_address_space;

    uint64_t pat = x86_64_msr_read(X86_64_MSR_PAT);
    pat &= ~(((uint64_t) 0b111 << 48) | ((uint64_t) 0b111 << 40));
    pat |= ((uint64_t) 0x1 << 48) | ((uint64_t) 0x5 << 40);
    x86_64_msr_write(X86_64_MSR_PAT, pat);

    uint64_t cr4;
    asm volatile("mov %%cr4, %0" : "=r" (cr4) : : "memory");
    cr4 |= 1 << 7; /* CR4.PGE */
    asm volatile("mov %0, %%cr4" : : "r" (cr4) : "memory");

    x86_64_lapic_initialize();
    x86_64_gdt_load();
    x86_64_interrupt_load_idt();

    // TODO: this will be replace with thread once we have a scheduler
    x86_64_msr_write(X86_64_MSR_GS_BASE, (uint64_t) cpu);

    arch_interrupt_ipl(ARCH_INTERRUPT_IPL_NORMAL);
    __atomic_add_fetch(&g_x86_64_cpus_initialized, 1, __ATOMIC_SEQ_CST);
}

[[noreturn]] __attribute__((naked)) void init_ap() {
    ADJUST_STACK(g_hhdm_base);
    arch_vmm_load_address_space(g_vmm_kernel_address_space);

    init_common();
    asm volatile("sti");

    for(;;) asm volatile("hlt");
    __builtin_unreachable();
}

[[noreturn]] void init(tartarus_boot_info_t *boot_info) {
    g_hhdm_base = boot_info->hhdm_base;
    g_hhdm_size = boot_info->hhdm_size;

    g_kprint_putchar = pch;

    kprintf("Elysium pre-alpha\n");

    for(uint16_t i = 0; i < boot_info->module_count; i++) {
        tartarus_module_t *module = &boot_info->modules[i];
        if(memcmp(module->name, "KERNSYMBTXT", 11) != 0) continue;
        g_panic_symbols = (char *) HHDM(module->paddr);
        g_panic_symbols_length = module->size;
    }

    // Initialize physical memory
    pmm_zone_register(PMM_ZONE_DMA, "DMA", 0, 0x100'0000);
    pmm_zone_register(PMM_ZONE_NORMAL, "Normal", 0x100'0000, UINTPTR_MAX);
    for(int i = 0; i < boot_info->memory_map_size; i++) {
        tartarus_mmap_entry_t entry = boot_info->memory_map[i];
        if(entry.type != TARTARUS_MEMAP_TYPE_USABLE) continue;
        pmm_region_add(entry.base, entry.length);
    }

    // Setup interrupts & exceptions (not enabled yet)
    x86_64_pic8259_remap();
    x86_64_pic8259_disable();
    g_x86_64_interrupt_irq_eoi = x86_64_lapic_eoi;
    x86_64_interrupt_init();
    for(int i = 0; i < 32; i++) {
        switch(i) {
            case 0xe:
                x86_64_interrupt_set(i, X86_64_INTERRUPT_PRIORITY_EXCEPTION, x86_64_vmm_page_fault_handler);
                break;
            default:
                x86_64_interrupt_set(i, X86_64_INTERRUPT_PRIORITY_EXCEPTION, x86_64_exception_unhandled);
                break;
        }
    }

    // Initialize virtual memory
    g_vmm_kernel_address_space = arch_vmm_init();

    g_hhdm_segment.address_space = g_vmm_kernel_address_space;
    g_hhdm_segment.base = ROUND_DOWN(g_hhdm_base, ARCH_PAGE_SIZE);
    g_hhdm_segment.length = ROUND_UP(g_hhdm_size, ARCH_PAGE_SIZE);
    g_hhdm_segment.protection = VMM_PROT_READ | VMM_PROT_WRITE;
    g_hhdm_segment.driver = &g_seg_prot;
    g_hhdm_segment.driver_data = "HHDM";
    list_append(&g_vmm_kernel_address_space->segments, &g_hhdm_segment.list_elem);

    g_kernel_segment.address_space = g_vmm_kernel_address_space;
    g_kernel_segment.base = ROUND_DOWN(boot_info->kernel_vaddr, ARCH_PAGE_SIZE);
    g_kernel_segment.length = ROUND_UP(boot_info->kernel_size, ARCH_PAGE_SIZE);
    g_kernel_segment.protection = VMM_PROT_READ | VMM_PROT_WRITE;
    g_kernel_segment.driver = &g_seg_prot;
    g_kernel_segment.driver_data = "Kernel";
    list_append(&g_vmm_kernel_address_space->segments, &g_kernel_segment.list_elem);

    ADJUST_STACK(g_hhdm_base);
    arch_vmm_load_address_space(g_vmm_kernel_address_space);

    // Common init
    init_common();
    asm volatile("sti");

    // Initialize heap
    heap_initialize(g_vmm_kernel_address_space, 0x100'0000'0000);

    // Wake AP's
    size_t max_cpus = sizeof(g_x86_64_cpus) / sizeof(x86_64_cpu_t);
    if(boot_info->cpu_count > max_cpus) kprintf("WARNING: This system contains more than %lu cpus, only %lu will be initialized.", max_cpus, max_cpus);
    for(size_t i = 0; i < (boot_info->cpu_count > max_cpus ? max_cpus : boot_info->cpu_count); i++) {
        if(i == boot_info->bsp_index) continue;

        size_t cur_init_count = g_x86_64_cpus_initialized;
        *boot_info->cpus[i].wake_on_write = (uint64_t) init_ap;
        while(cur_init_count >= g_x86_64_cpus_initialized);
    }

    /**
     *
     *
     * TESTING STUFF BELOW
     *  v  v  v  v  v  v
     *
     *
     */
    kprintf("\n");
    kprintf("Physical Memory Map\n");
    for(int i = 0; i <= PMM_ZONE_MAX; i++) {
        pmm_zone_t *zone = &g_pmm_zones[i];
        if(!zone->present) continue;

        kprintf("- %s\n", zone->name);
        LIST_FOREACH(&zone->regions, elem) {
            pmm_region_t *region = LIST_CONTAINER_GET(elem, pmm_region_t, list_elem);
            kprintf("  - %#-12lx %lu/%lu pages\n", region->base, region->free_count, region->page_count);
        }
    }

    void *random_addr = vmm_map(g_vmm_kernel_address_space, NULL, 0x5000, VMM_PROT_READ, VMM_FLAG_NONE, &g_seg_anon, NULL);
    ASSERT(random_addr != NULL);
    kprintf("\nVMM randomly allocated address: %#lx\n", random_addr);

    for(;;) asm volatile("hlt");
    __builtin_unreachable();
}