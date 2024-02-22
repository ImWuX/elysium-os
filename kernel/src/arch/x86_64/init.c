#include "init.h"
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <tartarus.h>
#include <lib/format.h>
#include <lib/math.h>
#include <lib/list.h>
#include <common/kprint.h>
#include <common/assert.h>
#include <memory/hhdm.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <memory/heap.h>
#include <arch/vmm.h>
#include <arch/types.h>
#include <arch/cpu.h>
#include <arch/interrupt.h>
#include <arch/x86_64/vmm.h>
#include <arch/x86_64/gdt.h>
#include <arch/x86_64/port.h>
#include <arch/x86_64/msr.h>
#include <arch/x86_64/lapic.h>
#include <arch/x86_64/interrupt.h>
#include <arch/x86_64/exception.h>
#include <arch/x86_64/dev/pic8259.h>

#define ADJUST_STACK(OFFSET) asm volatile("mov %%rsp, %%rax\nadd %0, %%rax\nmov %%rax, %%rsp\nmov %%rbp, %%rax\nadd %0, %%rax\nmov %%rax, %%rbp" : : "rm" (OFFSET) : "rax", "memory")

uintptr_t g_hhdm_base;
size_t g_hhdm_size;
static vmm_segment_t g_hhdm_segment, g_kernel_segment;
static x86_64_init_stage_t init_stage = X86_64_INIT_STAGE_ENTRY;

static void pch(char ch) {
	x86_64_port_outb(0x3F8, ch);
}

int kprintv(const char *fmt, va_list list) {
	return format(pch, fmt, list);
}

int kprintf(const char *fmt, ...) {
    va_list list;
	va_start(list, fmt);
	int ret = kprintv(fmt, list);
	va_end(list);
	return ret;
}

x86_64_init_stage_t x86_64_init_stage() {
    return init_stage;
}

static void set_init_stage(x86_64_init_stage_t stage) {
    init_stage = stage;
}

[[noreturn]] void init(tartarus_boot_info_t *boot_info) {
	g_hhdm_base = boot_info->hhdm_base;
	g_hhdm_size = boot_info->hhdm_size;

	kprintf("Elysium Alpha\n");
    kprintf("HHDM: %#lx (%#lx)\n", g_hhdm_base, g_hhdm_size);

    // GDT
    x86_64_gdt_load();

	// Initialize physical memory
    pmm_zone_register(PMM_ZONE_DMA, "DMA", 0, 0x100'0000);
    pmm_zone_register(PMM_ZONE_NORMAL, "Normal", 0x100'0000, UINTPTR_MAX);
    for(int i = 0; i < boot_info->memory_map_size; i++) {
        tartarus_mmap_entry_t entry = boot_info->memory_map[i];
        if(entry.type != TARTARUS_MEMAP_TYPE_USABLE) continue;
        pmm_region_add(entry.base, entry.length);
    }
    set_init_stage(X86_64_INIT_STAGE_PHYS_MEMORY);

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

    // Prep interrupts
    x86_64_pic8259_remap();
    x86_64_pic8259_disable();
    x86_64_lapic_initialize();
    g_x86_64_interrupt_irq_eoi = x86_64_lapic_eoi;
    x86_64_interrupt_init();
    x86_64_interrupt_load_idt();
    for(int i = 0; i < 32; i++) {
        switch(i) {
            default:
                x86_64_interrupt_set(i, X86_64_INTERRUPT_PRIORITY_EXCEPTION, x86_64_exception_unhandled);
                break;
        }
    }
    set_init_stage(X86_64_INIT_STAGE_INTERRUPTS);

    // Virtual Memory
    uint64_t pat = x86_64_msr_read(X86_64_MSR_PAT);
    pat &= ~(((uint64_t) 0b111 << 48) | ((uint64_t) 0b111 << 40));
    pat |= ((uint64_t) 0x1 << 48) | ((uint64_t) 0x5 << 40);
    x86_64_msr_write(X86_64_MSR_PAT, pat);

    uint64_t cr4;
    asm volatile("mov %%cr4, %0" : "=r" (cr4) : : "memory");
    cr4 |= 1 << 7; /* CR4.PGE */
    asm volatile("mov %0, %%cr4" : : "r" (cr4) : "memory");

    g_vmm_kernel_address_space = x86_64_vmm_init();

    g_hhdm_segment.address_space = g_vmm_kernel_address_space;
    g_hhdm_segment.base = MATH_FLOOR(g_hhdm_base, ARCH_PAGE_SIZE);
    g_hhdm_segment.length = MATH_CEIL(g_hhdm_size, ARCH_PAGE_SIZE);
    g_hhdm_segment.protection = VMM_PROT_READ | VMM_PROT_WRITE;
    g_hhdm_segment.driver = &g_seg_prot;
    g_hhdm_segment.driver_data = "HHDM";
    list_append(&g_vmm_kernel_address_space->segments, &g_hhdm_segment.list_elem);

    g_kernel_segment.address_space = g_vmm_kernel_address_space;
    g_kernel_segment.base = MATH_FLOOR(boot_info->kernel_vaddr, ARCH_PAGE_SIZE);
    g_kernel_segment.length = MATH_CEIL(boot_info->kernel_size, ARCH_PAGE_SIZE);
    g_kernel_segment.protection = VMM_PROT_READ | VMM_PROT_WRITE;
    g_kernel_segment.driver = &g_seg_prot;
    g_kernel_segment.driver_data = "Kernel";
    list_append(&g_vmm_kernel_address_space->segments, &g_kernel_segment.list_elem);

    ADJUST_STACK(g_hhdm_base);
    arch_vmm_load_address_space(g_vmm_kernel_address_space);

    x86_64_interrupt_set(0xE, X86_64_INTERRUPT_PRIORITY_EXCEPTION, x86_64_vmm_page_fault_handler);

    set_init_stage(X86_64_INIT_STAGE_MEMORY);

    void *random_addr = vmm_map(g_vmm_kernel_address_space, NULL, 0x5000, VMM_PROT_READ, VMM_FLAG_NONE, &g_seg_anon, NULL);
    ASSERT(random_addr != NULL);
    kprintf("\nVMM randomly allocated address: %#lx\n", random_addr);

    // Initialize heap
    heap_initialize(g_vmm_kernel_address_space, 0x100'0000'0000);

    void *p = heap_alloc(0x500);
    ASSERT(p != NULL);
    kprintf("\nHeap randomly allocated address: %#lx\n", p);

    // Enable interrupts
    arch_interrupt_set_ipl(IPL_NORMAL);
    asm volatile("sti");
    set_init_stage(X86_64_INIT_STAGE_FINAL);

    arch_cpu_halt();
}