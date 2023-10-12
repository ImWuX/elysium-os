#include <stdint.h>
#include <tartarus.h>
#include <string.h>
#include <cpuid.h>
#include <chaos.h>
#include <lib/panic.h>
#include <lib/assert.h>
#include <lib/kprint.h>
#include <lib/elf.h>
#include <sys/dev.h>
#include <sys/cpu.h>
#include <sched/sched.h>
#include <memory/hhdm.h>
#include <memory/heap.h>
#include <graphics/draw.h>
#include <graphics/tgarender.h>
#include <drivers/acpi.h>
#include <fs/vfs.h>
#include <fs/tmpfs.h>
#include <fs/rdsk.h>
#include <arch/vmm.h>
#include <arch/sched.h>
#include <arch/amd64/sched/sched.h>
#include <arch/amd64/cpu.h>
#include <arch/amd64/gdt.h>
#include <arch/amd64/msr.h>
#include <arch/amd64/port.h>
#include <arch/amd64/cpuid.h>
#include <arch/amd64/lapic.h>
#include <arch/amd64/exception.h>
#include <arch/amd64/interrupt.h>
#include <arch/amd64/drivers/pic8259.h>
#include <arch/amd64/drivers/ioapic.h>
#include <arch/amd64/drivers/ps2.h>
#include <arch/amd64/drivers/ps2kb.h>
#include <arch/amd64/drivers/ps2mouse.h>
#include <arch/amd64/drivers/pit.h>
#include <arch/amd64/sched/syscall.h>

#define LAPIC_CALIBRATION_TICKS 0x10000

uintptr_t g_hhdm_address;
draw_context_t g_fb_context;
static volatile int g_cpus_initialized;

uint32_t g_fpu_area_size = 0;
void (* g_fpu_save)(void *area) = 0;
void (* g_fpu_restore)(void *area) = 0;

static inline void xsave(void *area) {
    asm volatile ("xsave (%0)" : : "r" (area), "a"(0xffffffff), "d"(0xffffffff) : "memory");
}

static inline void xrstor(void *area) {
    asm volatile ("xrstor (%0)" : : "r" (area), "a"(0xffffffff), "d"(0xffffffff) : "memory");
}

static inline void fxsave(void *area) {
    asm volatile ("fxsave (%0)" : : "r" (area) : "memory");
}

static inline void fxrstor(void *area) {
    asm volatile ("fxrstor (%0)" : : "r" (area) : "memory");
}

static void fpu_setup() {
    /* Enable FPU */
    uint64_t cr0;
    asm volatile("mov %%cr0, %0" : "=r" (cr0) : : "memory");
    cr0 &= ~(1 << 2); /* CR0.EM */
    cr0 |= 1 << 1; /* CR0.MP */
    asm volatile("mov %0, %%cr0" : : "r" (cr0) : "memory");

    /* Enable MMX & friends */
    uint64_t cr4;
    asm volatile("mov %%cr4, %0" : "=r" (cr4) : : "memory");
    cr4 |= 1 << 9; /* CR4.OSFXSR */
    cr4 |= 1 << 10; /* CR4.OSXMMEXCPT */
    asm volatile("mov %0, %%cr4" : : "r" (cr4) : "memory");

    if(cpuid_feature(CPUID_FEATURE_XSAVE)) {
        asm volatile("mov %%cr4, %0" : "=r" (cr4) : : "memory");
        cr4 |= 1 << 18; /* CR4.OSXSAVE */
        asm volatile("mov %0, %%cr4" : : "r" (cr4) : "memory");

        uint64_t xcr0 = 0;
        xcr0 |= 1 << 0; /* XCR0.X87 */
        xcr0 |= 1 << 1; /* XCR0.SSE */
        if(cpuid_feature(CPUID_FEATURE_AVX)) xcr0 |= 1 << 2; /* XCR0.AVX */
        if(cpuid_feature(CPUID_FEATURE_AVX512)) {
            xcr0 |= 1 << 5; /* XCR0.opmask */
            xcr0 |= 1 << 6; /* XCR0.ZMM_Hi256 */
            xcr0 |= 1 << 7; /* XCR0.Hi16_ZMM */
        }
        asm volatile("xsetbv" : : "a" (xcr0), "d" (xcr0 >> 32), "c" (0) : "memory");
    }
}

[[noreturn]] static void init_common() {
    uint64_t pat = msr_read(MSR_PAT);
    pat &= ~(((uint64_t) 0b111 << 48) | ((uint64_t) 0b111 << 40));
    pat |= ((uint64_t) 0x1 << 48) | ((uint64_t) 0x5 << 40);
    msr_write(MSR_PAT, pat);

    pit_initialize(UINT16_MAX);
    uint16_t start_count = pit_count();
    lapic_timer_poll(LAPIC_CALIBRATION_TICKS);
    uint16_t end_count = pit_count();

    tss_t *tss = heap_alloc(sizeof(tss_t));
    memset(tss, 0, sizeof(tss_t));
    tss->iomap_base = sizeof(tss_t);
    gdt_load_tss(tss);

    arch_cpu_t *cpu = heap_alloc(sizeof(arch_cpu_t));
    cpu->lapic_timer_frequency = (uint64_t) (LAPIC_CALIBRATION_TICKS / (start_count - end_count)) * PIT_FREQ;
    cpu->tss = tss;
    cpu->lapic_id = lapic_id();

    syscall_init();

    asm volatile("sti");

    __atomic_add_fetch(&g_cpus_initialized, 1, __ATOMIC_SEQ_CST);

    sched_init_cpu(cpu);
    __builtin_unreachable();
}

[[noreturn]] __attribute__((naked)) void init_ap() {
    asm volatile("mov %%rsp, %%rax\nadd %0, %%rax\nmov %%rax, %%rsp" : : "r" (g_hhdm_address) : "rax", "memory");
    asm volatile("mov %%rbp, %%rax\nadd %0, %%rax\nmov %%rax, %%rbp" : : "r" (g_hhdm_address) : "rax", "memory");

    arch_vmm_load_address_space(&g_kernel_address_space);

    gdt_load();
    interrupt_load_idt();

    fpu_setup();

    ASSERT(cpuid_feature(CPUID_FEATURE_APIC));
    lapic_initialize();

    init_common();
    __builtin_unreachable();
}

[[noreturn]] extern void init(tartarus_boot_info_t *boot_info) {
    g_fb_context.address = boot_info->framebuffer.address;
    g_fb_context.width = boot_info->framebuffer.width;
    g_fb_context.height = boot_info->framebuffer.height;
    g_fb_context.pitch = boot_info->framebuffer.pitch;

    ASSERT(boot_info->hhdm_base >= ARCH_HHDM_START);
    ASSERT(boot_info->hhdm_base + boot_info->hhdm_size < ARCH_HHDM_END);
    g_hhdm_address = boot_info->hhdm_base;

    for(uint16_t i = 0; i < boot_info->module_count; i++) {
        tartarus_module_t *module = &boot_info->modules[i];
        if(memcmp(module->name, "KSYMB   TXT", 11) != 0) continue;
        g_panic_symbols = (char *) HHDM(module->paddr);
        g_panic_symbols_length = module->size;
    }

    pmm_zone_create(PMM_ZONE_INDEX_DMA, "DMA", 0, 0x100'0000);
    pmm_zone_create(PMM_ZONE_INDEX_NORMAL, "Normal", 0x100'0000, UINTPTR_MAX);
    for(int i = 0; i < boot_info->memory_map_size; i++) {
        tartarus_mmap_entry_t entry = boot_info->memory_map[i];
        if(entry.type != TARTARUS_MEMAP_TYPE_USABLE) continue;
        if(entry.base < 0x100'0000) {
            if(entry.base + entry.length > 0x100'0000) {
                pmm_region_add(PMM_ZONE_INDEX_DMA, entry.base, 0x100'0000 - entry.base);
                pmm_region_add(PMM_ZONE_INDEX_NORMAL, 0x100'0000, entry.length - (0x100'0000 - entry.base));
            } else {
                pmm_region_add(PMM_ZONE_INDEX_DMA, entry.base, entry.length);
            }
        } else {
            pmm_region_add(PMM_ZONE_INDEX_NORMAL, entry.base, entry.length);
        }
    }

    asm volatile("mov %%rsp, %%rax\nadd %0, %%rax\nmov %%rax, %%rsp" : : "rm" (g_hhdm_address) : "rax", "memory");
    asm volatile("mov %%rbp, %%rax\nadd %0, %%rax\nmov %%rax, %%rbp" : : "rm" (g_hhdm_address) : "rax", "memory");

    arch_vmm_init();
    arch_vmm_load_address_space(&g_kernel_address_space);
    heap_initialize(&g_kernel_address_space, ARCH_KHEAP_START, ARCH_KHEAP_END);

    gdt_load();

    ASSERT(cpuid_feature(CPUID_FEATURE_MSR));

    interrupt_initialize();
    for(int i = 0; i < 32; i++) {
        interrupt_set(i, INTERRUPT_PRIORITY_EXCEPTION, exception_unhandled);
    }
    interrupt_load_idt();

    fpu_setup();
    if(cpuid_feature(CPUID_FEATURE_XSAVE)) {
        uint32_t area_size;
        ASSERT(!cpuid_register(0xD, CPUID_REGISTER_ECX, &area_size));
        g_fpu_area_size = area_size;
        g_fpu_save = xsave;
        g_fpu_restore = xrstor;
    } else {
        g_fpu_area_size = 512;
        g_fpu_save = fxsave;
        g_fpu_restore = fxrstor;
    }

    ASSERT(cpuid_feature(CPUID_FEATURE_APIC));
    pic8259_remap();
    pic8259_disable();
    lapic_initialize();
    g_interrupt_irq_eoi = lapic_eoi;

    acpi_initialize(boot_info->acpi_rsdp);

    acpi_sdt_header_t *madt = acpi_find_table((uint8_t *) "APIC");
    if(madt) ioapic_initialize(madt);

    dev_initialize();

    acpi_fadt_t *fadt = (acpi_fadt_t *) acpi_find_table((uint8_t *) "FACP");
    if(fadt && (acpi_revision() == 0 || (fadt->boot_architecture_flags & (1 << 1)))) {
        // ps2kb_set_handler((ps2kb_handler_t) );
        // ps2mouse_set_handler((ps2mouse_handler_t) );
        ps2_initialize();
    }

    sched_init();

    int r = vfs_mount(&g_tmpfs_ops, 0, 0);
    if(r < 0) panic("Failed to mount tmpfs (%i)\n", r);
    vfs_node_t *modules_dir;
    r = vfs_mkdir("/", "modules", &modules_dir, 0);
    if(r < 0) panic("Failed to create /modules directory (%i)\n", r);
    r = vfs_mount(&g_tmpfs_ops, "/modules", (void *) 0);
    if(r < 0) panic("Failed to mount /modules (%i)\n", r);

    tartarus_module_t *sroot_module = 0;
    for(uint16_t i = 0; i < boot_info->module_count; i++) {
        tartarus_module_t *module = &boot_info->modules[i];
        if(strcmp("SROOT   RDK", module->name) == 0) {
            sroot_module = module;
            continue;
        }
        vfs_node_t *file;
        r = vfs_create("/modules", module->name, &file, 0);
        if(r < 0) continue;
        vfs_rw_t rw = { .rw = VFS_RW_WRITE, .size = module->size, .buffer = (void *) HHDM(module->paddr) };
        size_t write_count;
        r = file->ops->rw(file, &rw, &write_count);
        if(r < 0 || write_count != module->size) panic("Failed to write module to tmpfs file (%s)\n", module->name);
    }

    if(sroot_module) {
        vfs_node_t *sysroot_dir;
        r = vfs_mkdir("/", "sysroot", &sysroot_dir, 0);
        if(r < 0) panic("Failed to create /sysroot directory (%s)\n", r);
        r = vfs_mount(&g_rdsk_ops, "/sysroot", (void *) HHDM(sroot_module->paddr));
        if(r < 0) panic("Failed to mount /sysroot (%s)\n", r);
    } else {
        panic("No sroot.rdk\n");
    }

    {
        vmm_address_space_t *as = arch_vmm_fork(&g_kernel_address_space);
        vfs_node_t *startup_exec;
        r = vfs_lookup("/modules/STYX    ELF", &startup_exec, 0);
        if(r < 0) panic("Could not lookup the startup executable (%i)\n", r);

        char *interpreter = 0;
        elf_auxv_t auxv;
        bool elf_r = elf_load(startup_exec, as, &interpreter, &auxv);
        if(elf_r) panic("Could not load the startup executable\n");
        uintptr_t stack = arch_vmm_highest_userspace_addr();
        for(size_t j = 0; j < 8; j++) arch_vmm_map(as, (stack & ~0xFFF) - ARCH_PAGE_SIZE * j, pmm_alloc_page(PMM_GENERAL | PMM_AF_ZERO)->paddr, VMM_FLAGS_WRITE | VMM_FLAGS_USER);
        process_t *proc = sched_process_create(as);
        thread_t *thread = arch_sched_thread_create_user(proc, auxv.entry, stack);
        sched_thread_schedule(thread);
    }

    {
        bool elf_r;

        vmm_address_space_t *as = arch_vmm_fork(&g_kernel_address_space);
        vfs_node_t *startup_exec;
        r = vfs_lookup("/modules/TCTEST  ELF", &startup_exec, 0);
        if(r < 0) panic("Could not lookup tctest.elf (%i)\n", r);

        elf_auxv_t auxv = {};
        char *interpreter = 0;
        elf_r = elf_load(startup_exec, as, &interpreter, &auxv);
        if(elf_r) panic("Could not load tctest.elf\n");
        kprintf("Found interpreter: %s\n", interpreter);

        elf_auxv_t interp_auxv = {};
        if(interpreter) {
            char temp[1024]; // TODO: TEMP
            memcpy(temp, "/sysroot", 8);
            strcpy(temp + 8, interpreter);
            kprintf("acutally found it? %s\n", temp);
            vfs_node_t *interp_exec;
            r = vfs_lookup(temp, &interp_exec, 0);
            if(r < 0) panic("Could not lookup the interpreter for startup (%i)\n", r);

            elf_r = elf_load(interp_exec, as, 0, &interp_auxv);
            if(elf_r) panic("Could not load the interpreter for startup\n");
        }

        uintptr_t stack = arch_vmm_highest_userspace_addr();
        uintptr_t paddr;
        for(size_t j = 0; j < 8; j++) {
            paddr = pmm_alloc_page(PMM_GENERAL | PMM_AF_ZERO)->paddr;
            arch_vmm_map(as, (stack & ~0xFFF) - ARCH_PAGE_SIZE * (7 - j), paddr, VMM_FLAGS_WRITE | VMM_FLAGS_USER);
        }

        stack &= ~0xF;
        uint64_t *stackp = (uint64_t *) ((HHDM(paddr) + ARCH_PAGE_SIZE - 1) & ~0xF);

        stackp -= 2; stack -= sizeof(uint64_t) * 2;
        memcpy((void *) stackp, (void *) "tctest.elf", 11);
        uintptr_t stack_w_str = stack;

        stackp -= 2; stack -= sizeof(uint64_t) * 2;
        memcpy((void *) stackp, (void *) "LD_SHOW_AUXV=1", 15);
        uintptr_t env_ld_show_auxv = stack;

        stackp -= 3; stack -= sizeof(uint64_t) * 3;
        stackp[0] = 0; stackp[1] = 0; stackp[2] = 0;

        stackp -= 2; stack -= sizeof(uint64_t) * 2;
        stackp[0] = 23; stackp[1] = 0;

        stackp -= 2; stack -= sizeof(uint64_t) * 2;
        stackp[0] = ELF_AUXV_ENTRY; stackp[1] = auxv.entry;

        stackp -= 2; stack -= sizeof(uint64_t) * 2;
        stackp[0] = ELF_AUXV_PHDR; stackp[1] = auxv.phdr;

        stackp -= 2; stack -= sizeof(uint64_t) * 2;
        stackp[0] = ELF_AUXV_PHENT; stackp[1] = auxv.phent;

        stackp -= 2; stack -= sizeof(uint64_t) * 2;
        stackp[0] = ELF_AUXV_PHNUM; stackp[1] = auxv.phnum;

        stackp -= 1; stack -= sizeof(uint64_t) * 1;
        stackp[0] = 0; // ENVP NULL

        stackp -= 1; stack -= sizeof(uint64_t) * 1;
        stackp[0] = env_ld_show_auxv; // envp env_ld_show_auxv

        stackp -= 1; stack -= sizeof(uint64_t) * 1;
        stackp[0] = 0; // ARG NULL

        stackp -= 1; stack -= sizeof(uint64_t) * 1;
        stackp[0] = stack_w_str; // call arg

        stackp -= 1; stack -= sizeof(uint64_t) * 1;
        stackp[0] = 1; // argc

        kprintf("entry: %#lx; phdr: %#lx; phent: %#lx; phnum: %#lx;\n", auxv.entry, auxv.phdr, auxv.phent, auxv.phnum);
        kprintf("stack: %#lx; stackp: %#lx;\n", stack, (uintptr_t) stackp);

        process_t *proc = sched_process_create(as);
        thread_t *thread = arch_sched_thread_create_user(proc, interpreter ? interp_auxv.entry : auxv.entry, stack);
        sched_thread_schedule(thread);
    }

    chaos_tests_init();

    g_cpus_initialized = 0;
    for(int i = 0; i < boot_info->cpu_count; i++) {
        if(i == boot_info->bsp_index) {
            g_cpus_initialized++;
            continue;
        }
        *boot_info->cpus[i].wake_on_write = (uint64_t) &init_ap;
        while(i >= g_cpus_initialized);
    }

    init_common();
    __builtin_unreachable();
}

// TODO: this has got to be replaced by a proper logging subsystem
int putchar(int ch) {
    port_outb(0x3F8, (char) ch);
    return (char) ch;
}