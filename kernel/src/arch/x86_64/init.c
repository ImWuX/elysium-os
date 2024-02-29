#include "init.h"
#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>
#include <errno.h>
#include <tartarus.h>
#include <lib/format.h>
#include <lib/math.h>
#include <lib/list.h>
#include <lib/mem.h>
#include <lib/str.h>
#include <common/log.h>
#include <common/elf.h>
#include <common/assert.h>
#include <common/panic.h>
#include <memory/hhdm.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <memory/heap.h>
#include <fs/vfs.h>
#include <fs/tmpfs.h>
#include <fs/rdsk.h>
#include <sched/sched.h>
#include <arch/vmm.h>
#include <arch/types.h>
#include <arch/cpu.h>
#include <arch/sched.h>
#include <arch/interrupt.h>
#include <arch/x86_64/vmm.h>
#include <arch/x86_64/interrupt.h>
#include <arch/x86_64/exception.h>
#include <arch/x86_64/sched.h>
#include <arch/x86_64/sys/tss.h>
#include <arch/x86_64/sys/gdt.h>
#include <arch/x86_64/sys/port.h>
#include <arch/x86_64/sys/fpu.h>
#include <arch/x86_64/sys/msr.h>
#include <arch/x86_64/sys/cpu.h>
#include <arch/x86_64/sys/cpuid.h>
#include <arch/x86_64/sys/lapic.h>
#include <arch/x86_64/syscall.h>
#include <arch/x86_64/dev/pit.h>
#include <arch/x86_64/dev/pic8259.h>

#define LAPIC_CALIBRATION_TICKS 0x10000
#define ADJUST_STACK(OFFSET) asm volatile("mov %%rsp, %%rax\nadd %0, %%rax\nmov %%rax, %%rsp\nmov %%rbp, %%rax\nadd %0, %%rax\nmov %%rax, %%rbp" : : "rm" (OFFSET) : "rax", "memory")

uintptr_t g_hhdm_base;
size_t g_hhdm_size;

static vmm_segment_t g_hhdm_segment, g_kernel_segment;
static x86_64_init_stage_t init_stage = X86_64_INIT_STAGE_ENTRY;

volatile size_t g_x86_64_cpu_count;
x86_64_cpu_t *g_x86_64_cpus;

static void log_raw_serial(char c) {
	x86_64_port_outb(0x3F8, c);
}

static void format_lrs(char *fmt, ...) {
    va_list list;
	va_start(list, fmt);
    format(log_raw_serial, fmt, list);
	va_end(list);
}

static void log_serial(log_level_t level, const char *tag, const char *fmt, va_list args) {
    char *color;
    switch(level) {
        case LOG_LEVEL_DEBUG: color = "\e[36m"; break;
        case LOG_LEVEL_INFO: color = "\e[33m"; break;
        case LOG_LEVEL_WARN: color = "\e[91m"; break;
        case LOG_LEVEL_ERROR: color = "\e[31m"; break;
        default: color = "\e[0m"; break;
    }
    format_lrs("%s[%s::%s]%s ", color, log_level_tostring(level), tag, "\e[0m");
    format(log_raw_serial, fmt, args);
    log_raw_serial('\n');
}

static log_sink_t g_serial_sink = {
    .name = "SERIAL",
    .level = LOG_LEVEL_DEBUG,
    .log = log_serial,
    .log_raw = log_raw_serial
};

// TODO: A decent amount of duplicate code here. Consider having some sort of shared init.
[[noreturn]] __attribute__((naked)) static void init_ap() {
    log(LOG_LEVEL_INFO, "INIT", "AP %i:%i Init", g_x86_64_cpu_count, x86_64_lapic_id());

    x86_64_gdt_load();

    // Virtual Memory
    uint64_t pat = x86_64_msr_read(X86_64_MSR_PAT);
    pat &= ~(((uint64_t) 0b111 << 48) | ((uint64_t) 0b111 << 40));
    pat |= ((uint64_t) 0x1 << 48) | ((uint64_t) 0x5 << 40);
    x86_64_msr_write(X86_64_MSR_PAT, pat);

    uint64_t cr4;
    asm volatile("mov %%cr4, %0" : "=r" (cr4) : : "memory");
    cr4 |= 1 << 7; /* CR4.PGE */
    asm volatile("mov %0, %%cr4" : : "r" (cr4) : "memory");

    ADJUST_STACK(g_hhdm_base);
    arch_vmm_load_address_space(g_vmm_kernel_address_space);

    // Interrupts
    ASSERT(x86_64_cpuid_feature(X86_64_CPUID_FEATURE_APIC));
    x86_64_lapic_initialize();
    x86_64_interrupt_load_idt();

    // CPU Local
    x86_64_tss_t *tss = heap_alloc(sizeof(x86_64_tss_t));
    memset(tss, 0, sizeof(x86_64_tss_t));
    tss->iomap_base = sizeof(x86_64_tss_t);
    x86_64_gdt_load_tss(tss);

    x86_64_pit_initialize(UINT16_MAX);
    uint16_t start_count = x86_64_pit_count();
    x86_64_lapic_timer_poll(LAPIC_CALIBRATION_TICKS);
    uint16_t end_count = x86_64_pit_count();

    x86_64_cpu_t *cpu = &g_x86_64_cpus[g_x86_64_cpu_count];
    cpu->lapic_id = x86_64_lapic_id();
    cpu->lapic_timer_frequency = (uint64_t) (LAPIC_CALIBRATION_TICKS / (start_count - end_count)) * PIT_FREQ;
    cpu->tss = tss;

    // Misc
    x86_64_fpu_init_cpu();
    x86_64_syscall_init_cpu();

    log(LOG_LEVEL_DEBUG, "INIT", "AP %i:%i init exit", g_x86_64_cpu_count, x86_64_lapic_id());
    __atomic_add_fetch(&g_x86_64_cpu_count, 1, __ATOMIC_SEQ_CST);

    arch_interrupt_set_ipl(IPL_SCHED);
    asm volatile("sti");
    x86_64_sched_init_cpu(cpu, false);
    __builtin_unreachable();
}

x86_64_init_stage_t x86_64_init_stage() {
    return init_stage;
}

void x86_64_init_stage_set(x86_64_init_stage_t stage) {
    init_stage = stage;
}

[[noreturn]] void init(tartarus_boot_info_t *boot_info) {
	g_hhdm_base = boot_info->hhdm_base;
	g_hhdm_size = boot_info->hhdm_size;

    log_sink_add(&g_serial_sink);
    log_raw('\n');
    log(LOG_LEVEL_INFO, "INIT", "Elysium Alpha");

    for(uint16_t i = 0; i < boot_info->module_count; i++) {
        tartarus_module_t *module = &boot_info->modules[i];
        if(strcmp("KERNSYMBTXT", module->name) != 0) continue;
        g_panic_symbols = (char *) HHDM(module->paddr);
        g_panic_symbols_length = module->size;
    }

    log(LOG_LEVEL_DEBUG, "HHDM", "Base: %#lx, Size: %#lx", g_hhdm_base, g_hhdm_size);

    ASSERT(x86_64_cpuid_feature(X86_64_CPUID_FEATURE_MSR));

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
    x86_64_init_stage_set(X86_64_INIT_STAGE_PHYS_MEMORY);

    log(LOG_LEVEL_DEBUG, "PMEM", "Physical Memory Map");
    for(int i = 0; i <= PMM_ZONE_MAX; i++) {
        pmm_zone_t *zone = &g_pmm_zones[i];
        if(!zone->present) continue;

        log(LOG_LEVEL_DEBUG, "PMEM", "- %s", zone->name);
        LIST_FOREACH(&zone->regions, elem) {
            pmm_region_t *region = LIST_CONTAINER_GET(elem, pmm_region_t, list_elem);
            log(LOG_LEVEL_DEBUG, "PMEM", "  - %#-12lx %lu/%lu pages", region->base, region->free_count, region->page_count);
        }
    }

    // Initialize interrupts
    ASSERT(x86_64_cpuid_feature(X86_64_CPUID_FEATURE_APIC))
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
    x86_64_init_stage_set(X86_64_INIT_STAGE_INTERRUPTS);

    // Initialize Virtual Memory
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

    x86_64_init_stage_set(X86_64_INIT_STAGE_MEMORY);

    void *vmm_random_addr = vmm_map(g_vmm_kernel_address_space, NULL, 0x5000, VMM_PROT_READ, VMM_FLAG_NONE, &g_seg_anon, NULL);
    ASSERT(vmm_random_addr != NULL);
    log(LOG_LEVEL_DEBUG, "VMM", "randomly allocated & mapped address: %#lx", (uintptr_t) vmm_random_addr);

    // Initialize heap
    heap_initialize(g_vmm_kernel_address_space, 0x100'0000'0000);

    void *heap_random_address = heap_alloc(0x500);
    ASSERT(heap_random_address != NULL);
    log(LOG_LEVEL_DEBUG, "HEAP", "randomly allocated memory (0x500 bytes): %#lx", (uintptr_t) heap_random_address);

    // Initialize FPU
    x86_64_fpu_init();
    x86_64_fpu_init_cpu();

    // Initialize sched
    x86_64_sched_init();
    x86_64_syscall_init_cpu();

    // Initialize VFS
    for(uint16_t i = 0; i < boot_info->module_count; i++) log(LOG_LEVEL_DEBUG, "MODULES", "%s :: paddr: %#lx, size: %#lx", boot_info->modules[i].name, boot_info->modules[i].paddr, boot_info->modules[i].size);

    tartarus_module_t *sysroot_module = NULL;
    for(uint16_t i = 0; i < boot_info->module_count; i++) {
        tartarus_module_t *module = &boot_info->modules[i];
        if(strcmp("ROOT    RDK", module->name) != 0) continue;
        sysroot_module = module;
        break;
    }
    ASSERT(sysroot_module != NULL);

    int r = vfs_mount(&g_rdsk_ops, NULL, (void *) HHDM(sysroot_module->paddr));
    if(r < 0) panic("Failed to mount RDSK (%i)", r);

    vfs_t *root_vfs = LIST_CONTAINER_GET(LIST_NEXT(&g_vfs_all), vfs_t, list);
    vfs_node_t *root_node;
    ASSERT(root_vfs->ops->root(root_vfs, &root_node) == 0);
    log(LOG_LEVEL_DEBUG, "FS", "Root Directory");
    for(int i = 0;;) {
        char *filename;
        r = root_node->ops->readdir(root_node, &i, &filename);
        ASSERT(r == 0);
        if(filename == NULL) break;
        log(LOG_LEVEL_DEBUG, "FS", " - %s", filename);
    }

    r = vfs_mount(&g_tmpfs_ops, "/modules", NULL);
    if(r < 0) panic("Failed to mount /modules (%i)", r);
    r = vfs_mount(&g_tmpfs_ops, "/tmp", NULL);
    if(r < 0) panic("Failed to mount /tmp (%i)", r);

    for(uint16_t i = 0; i < boot_info->module_count; i++) {
        tartarus_module_t *module = &boot_info->modules[i];
        if(strcmp("ROOT    RDK", module->name) == 0) continue;

        vfs_node_t *file;
        r = vfs_create("/modules", module->name, &file, NULL);
        if(r < 0) continue;

        size_t write_count;
        r = file->ops->rw(file, &(vfs_rw_t) { .rw = VFS_RW_WRITE, .size = module->size, .buffer = (void *) HHDM(module->paddr) }, &write_count);
        if(r < 0 || write_count != module->size) panic("Failed to write module to tmpfs file (%s)", module->name);
    }

    // Load init
    {
        bool elf_r;

        vmm_address_space_t *as = arch_vmm_address_space_create();

        vfs_node_t *startup_exec;
        r = vfs_lookup("/usr/bin/init", &startup_exec, 0);
        if(r < 0) panic("Could not lookup test executable (%i)", r);

        auxv_t auxv = {};
        char *interpreter = 0;
        elf_r = elf_load(startup_exec, as, &interpreter, &auxv);
        if(elf_r) panic("Could not load test executable");

        auxv_t interp_auxv = {};
        if(interpreter) {
            log(LOG_LEVEL_DEBUG, "INIT", "Found init interpreter: %s", interpreter);
            vfs_node_t *interp_exec;
            r = vfs_lookup(interpreter, &interp_exec, 0);
            if(r < 0) panic("Could not lookup the interpreter for startup (%i)", r);

            elf_r = elf_load(interp_exec, as, 0, &interp_auxv);
            if(elf_r) panic("Could not load the interpreter for startup");
        }

        log(LOG_LEVEL_DEBUG, "INIT", "entry: %#lx; phdr: %#lx; phent: %#lx; phnum: %#lx;", auxv.entry, auxv.phdr, auxv.phent, auxv.phnum);

        char *argv[] = { "/usr/bin/init", NULL };
        char *envp[] = { NULL };

        process_t *proc = sched_process_create(as);
        uintptr_t thread_stack = arch_sched_stack_setup(proc, argv, envp, &auxv);
        thread_t *thread = arch_sched_thread_create_user(proc, interpreter ? interp_auxv.entry : auxv.entry, thread_stack);
        log(LOG_LEVEL_DEBUG, "INIT", "init thread >> entry: %#lx, stack: %#lx", interpreter ? interp_auxv.entry : auxv.entry, thread_stack);
        sched_thread_schedule(thread);
    }

    // SMP init
    g_x86_64_cpus = heap_alloc(sizeof(x86_64_cpu_t) * boot_info->cpu_count);

    x86_64_tss_t *tss = heap_alloc(sizeof(x86_64_tss_t));
    memset(tss, 0, sizeof(x86_64_tss_t));
    tss->iomap_base = sizeof(x86_64_tss_t);
    x86_64_gdt_load_tss(tss);

    x86_64_pit_initialize(UINT16_MAX);
    uint16_t start_count = x86_64_pit_count();
    x86_64_lapic_timer_poll(LAPIC_CALIBRATION_TICKS);
    uint16_t end_count = x86_64_pit_count();

    x86_64_cpu_t *cpu = &g_x86_64_cpus[boot_info->bsp_index];
    cpu->lapic_id = x86_64_lapic_id();
    cpu->lapic_timer_frequency = (uint64_t) (LAPIC_CALIBRATION_TICKS / (start_count - end_count)) * PIT_FREQ;
    cpu->tss = tss;

    g_x86_64_cpu_count = 0;
    for(size_t i = 0; i < boot_info->cpu_count; i++) {
        if(i == boot_info->bsp_index) {
            g_x86_64_cpu_count++;
            continue;
        }
        *boot_info->cpus[i].wake_on_write = (uint64_t) init_ap;
        while(i >= g_x86_64_cpu_count);
    }
    log(LOG_LEVEL_DEBUG, "INIT", "BSP init exit (%i/%i cpus initialized)", g_x86_64_cpu_count, boot_info->cpu_count);
    x86_64_init_stage_set(X86_64_INIT_STAGE_SMP);

    // Enable interrupts & handoff to sched
    arch_interrupt_set_ipl(IPL_SCHED);
    asm volatile("sti");
    x86_64_init_stage_set(X86_64_INIT_STAGE_PRE_SCHED);

    x86_64_sched_init_cpu(cpu, true);
    __builtin_unreachable();
}
