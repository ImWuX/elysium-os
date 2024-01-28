#include <tartarus.h>
#include <memory/hhdm.h>
#include <memory/pmm.h>
#include <memory/heap.h>
#include <lib/kprint.h>
#include <lib/panic.h>
#include <lib/string.h>
#include <lib/list.h>
#include <lib/assert.h>
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

arch_cpu_t g_cpus[256] = {};
volatile size_t g_cpus_initialized = 0;

static void pch(char ch) {
    port_outb(0x3F8, ch);
}

static void init_common() {
    arch_cpu_t *cpu = &g_cpus[g_cpus_initialized];
    memset(cpu, 0, sizeof(arch_cpu_t));
    cpu->this = cpu;
    cpu->lapic_id = lapic_id();
    cpu->tlb_shootdown_check = SPINLOCK_INIT;
    cpu->tlb_shootdown_lock = SPINLOCK_INIT;
    cpu->common.address_space = g_vmm_kernel_address_space;

    uint64_t pat = msr_read(MSR_PAT);
    pat &= ~(((uint64_t) 0b111 << 48) | ((uint64_t) 0b111 << 40));
    pat |= ((uint64_t) 0x1 << 48) | ((uint64_t) 0x5 << 40);
    msr_write(MSR_PAT, pat);

    uint64_t cr4;
    asm volatile("mov %%cr4, %0" : "=r" (cr4) : : "memory");
    cr4 |= 1 << 7; /* CR4.PGE */
    asm volatile("mov %0, %%cr4" : : "r" (cr4) : "memory");

    lapic_initialize();
    gdt_load();
    interrupt_load_idt();

    // TODO: this will be replace with thread once we have a scheduler
    msr_write(MSR_GS_BASE, (uint64_t) cpu);

    arch_interrupt_ipl(ARCH_INTERRUPT_IPL_NORMAL);

    __atomic_add_fetch(&g_cpus_initialized, 1, __ATOMIC_SEQ_CST);
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

    g_kprint_putchar = pch;

    kprintf("Elysium pre-alpha\n");

    for(uint16_t i = 0; i < boot_info->module_count; i++) {
        tartarus_module_t *module = &boot_info->modules[i];
        if(memcmp(module->name, "KERNSYMBTXT", 11) != 0) continue;
        g_panic_symbols = (char *) HHDM(module->paddr);
        g_panic_symbols_length = module->size;
    }

    pmm_zone_register(PMM_ZONE_DMA, "DMA", 0, 0x100'0000);
    pmm_zone_register(PMM_ZONE_NORMAL, "Normal", 0x100'0000, UINTPTR_MAX);
    for(int i = 0; i < boot_info->memory_map_size; i++) {
        tartarus_mmap_entry_t entry = boot_info->memory_map[i];
        if(entry.type != TARTARUS_MEMAP_TYPE_USABLE) continue;
        pmm_region_add(entry.base, entry.length);
    }

    pic8259_remap();
    pic8259_disable();
    g_interrupt_irq_eoi = lapic_eoi;
    interrupt_init();
    for(int i = 0; i < 32; i++) {
        switch(i) {
            case 0xe:
                interrupt_set(i, INTERRUPT_PRIORITY_EXCEPTION, vmm_page_fault_handler);
                break;
            default:
                interrupt_set(i, INTERRUPT_PRIORITY_EXCEPTION, exception_unhandled);
                break;
        }
    }

    arch_vmm_init();
    ADJUST_STACK(g_hhdm_base);
    arch_vmm_load_address_space(g_vmm_kernel_address_space);

    size_t max_cpus = sizeof(g_cpus) / sizeof(arch_cpu_t);
    g_cpus_initialized = 0;
    if(boot_info->cpu_count > max_cpus) kprintf("WARNING: This system contains more than %lu cpus, only %lu will be initialized.", max_cpus, max_cpus);
    for(size_t i = 0; i < (boot_info->cpu_count > max_cpus ? max_cpus : boot_info->cpu_count); i++) {
        if(i == boot_info->bsp_index) {
            init_common();
            continue;
        }
        *boot_info->cpus[i].wake_on_write = (uint64_t) init_ap;
        while(i >= g_cpus_initialized);
    }

    asm volatile("sti");

    heap_initialize(g_vmm_kernel_address_space, 0xFFFF'8400'0000'0000, 0xFFFF'8500'0000'0000);

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
    kprintf("\nVMM randomly allocated address: %#lx\n", random_addr);

    kprintf("MAP 1\n");
    LIST_FOREACH(&g_vmm_kernel_address_space->segments, elem) { vmm_segment_t *segment = LIST_CONTAINER_GET(elem, vmm_segment_t, list_elem); kprintf("-- (%s) %#lx + %#lx\n", segment->driver->name, segment->base, segment->length); }

    vmm_unmap(g_vmm_kernel_address_space, random_addr + 0x2000, 0x1000);
    kprintf("MAP 2\n");
    LIST_FOREACH(&g_vmm_kernel_address_space->segments, elem2) { vmm_segment_t *segment = LIST_CONTAINER_GET(elem2, vmm_segment_t, list_elem); kprintf("-- (%s) %#lx + %#lx\n", segment->driver->name, segment->base, segment->length); }

    vmm_unmap(g_vmm_kernel_address_space, random_addr + 0x4000, 0x1000);
    kprintf("MAP 3\n");
    LIST_FOREACH(&g_vmm_kernel_address_space->segments, elem3) { vmm_segment_t *segment = LIST_CONTAINER_GET(elem3, vmm_segment_t, list_elem); kprintf("-- (%s) %#lx + %#lx\n", segment->driver->name, segment->base, segment->length); }

    for(;;) asm volatile("hlt");
    __builtin_unreachable();
}