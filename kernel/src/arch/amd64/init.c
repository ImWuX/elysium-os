#include <tartarus.h>
#include <string.h>
#include <stdio.h>
#include <cpuid.h>
#include <panic.h>
#include <common.h>
#include <memory/hhdm.h>
#include <memory/heap.h>
#include <drivers/acpi.h>
#include <arch/vmm.h>
#include <arch/amd64/vmm.h>
#include <arch/amd64/gdt.h>
#include <arch/amd64/msr.h>
#include <arch/amd64/cpuid.h>
#include <arch/amd64/lapic.h>
#include <arch/amd64/interrupt.h>
#include <arch/amd64/sched.h>
#include <arch/amd64/drivers/pic8259.h>
#include <arch/amd64/drivers/ioapic.h>
#include <arch/amd64/drivers/ps2.h>
#include <arch/amd64/drivers/ps2kb.h>
#include <arch/amd64/drivers/pit.h>
#include <graphics/draw.h>
#include <graphics/basicfont.h>

#define TAB_WIDTH 4
#define INDENT 50
#define LAPIC_CALIBRATION_TICKS 0x10000

uintptr_t g_hhdm_address;
static vmm_address_space_t g_kernel_address_space;
static volatile int g_cpus_initialized;
static draw_context_t g_ctx;
static int g_x = INDENT, g_y = INDENT;
static volatile int fb_lock = 0;

int putchar(int c) {
    switch(c) {
        case '\t':
            g_x += (TAB_WIDTH - (g_x / BASICFONT_WIDTH) % TAB_WIDTH) * BASICFONT_WIDTH;
            break;
        case '\b':
            g_x -= BASICFONT_WIDTH;
            if(g_x < 0) g_x = 0;
            draw_rect(&g_ctx, g_x, g_y, BASICFONT_WIDTH, BASICFONT_HEIGHT, draw_color(20, 20, 25));
            break;
        case '\n':
            g_x = INDENT;
            g_y += BASICFONT_HEIGHT;
            break;
        default:
            draw_char(&g_ctx, g_x, g_y, (char) c, 0xFFFFFF);
            g_x += BASICFONT_WIDTH;
            break;
    }
    if(g_x >= g_ctx.width - INDENT) {
        g_x = INDENT;
        g_y += BASICFONT_HEIGHT;
    }
    if(g_y >= g_ctx.height - INDENT) {
        draw_rect(&g_ctx, 0, 0, g_ctx.width, g_ctx.height, draw_color(20, 20, 25));
        g_x = INDENT;
        g_y = INDENT;
    }
    return (char) c;
}

static void test_kb(uint8_t c) {
    putchar(c);
}

static void test_thread_loop() {
    while(true) {
        for(volatile uint64_t i1 = 0; i1 < 0xFFFFF; i1++);
        while(!__sync_bool_compare_and_swap(&fb_lock, 0, 1));
        printf(">%i ", lapic_id());
        __atomic_store_n(&fb_lock, 0, __ATOMIC_SEQ_CST);
    }
}

static void init_common() {
    uint64_t pat = msr_read(MSR_PAT);
    pat &= ~(((uint64_t) 0b111 << 48) | ((uint64_t) 0b111 << 40));
    pat |= ((uint64_t) 0x1 << 48) | ((uint64_t) 0x5 << 40);
    msr_write(MSR_PAT, pat);

    pit_initialize(UINT16_MAX);
    uint16_t start_count = pit_count();
    lapic_timer_poll(LAPIC_CALIBRATION_TICKS);
    uint16_t end_count = pit_count();

    arch_cpu_local_t *cpu_local = heap_alloc(sizeof(arch_cpu_local_t));
    cpu_local->lapic_timer_freq = (uint64_t) (LAPIC_CALIBRATION_TICKS / (start_count - end_count)) * PIT_FREQ;

    sched_thread_t *thread = heap_alloc(sizeof(sched_thread_t));
    thread->this = thread;
    thread->lock = 1;
    thread->cpu_local = cpu_local;
    sched_set_current_thread(thread);

    printf("@%i ", lapic_id());

    asm volatile("sti");

    lapic_timer_oneshot(g_sched_vector, 1'000'000);
}

[[noreturn]] __attribute__((naked)) void init_ap() {
    asm volatile("mov %%rsp, %%rax\nadd %0, %%rax\nmov %%rax, %%rsp" : : "r" (g_hhdm_address) : "rax", "memory");
    asm volatile("mov %%rbp, %%rax\nadd %0, %%rax\nmov %%rax, %%rbp" : : "r" (g_hhdm_address) : "rax", "memory");

    gdt_initialize();
    interrupt_load_idt();

    if(!cpuid_feature(CPUID_FEAT_APIC)) panic("ARCH/AMD64/INITAP", "Non-APIC CPUs are not supported");
    lapic_initialize();

    init_common();

    g_cpus_initialized++;
    for(;;) asm volatile("hlt");
    __builtin_unreachable();
}

[[noreturn]] __attribute__((naked)) extern void init(tartarus_boot_info_t *boot_info) {
    if(boot_info->hhdm_base < ARCH_HHDM_START || boot_info->hhdm_base + boot_info->hhdm_size >= ARCH_HHDM_END) panic("KERNEL", "HHDM is not within arch specific boundaries");
    g_hhdm_address = boot_info->hhdm_base;

    g_ctx.address = boot_info->framebuffer.address;
    g_ctx.width = boot_info->framebuffer.width;
    g_ctx.height = boot_info->framebuffer.height;
    g_ctx.pitch = boot_info->framebuffer.pitch;

    for(int i = 0; i < boot_info->memory_map_size; i++) {
        tartarus_mmap_entry_t entry = boot_info->memory_map[i];
        if(entry.type != TARTARUS_MEMAP_TYPE_USABLE) continue;
        pmm_region_add(entry.base, entry.length);
    }

    asm volatile("mov %%rsp, %%rax\nadd %0, %%rax\nmov %%rax, %%rsp" : : "rm" (g_hhdm_address) : "rax", "memory");
    asm volatile("mov %%rbp, %%rax\nadd %0, %%rax\nmov %%rax, %%rbp" : : "rm" (g_hhdm_address) : "rax", "memory");

    vmm_create_kernel_address_space(&g_kernel_address_space);
    arch_vmm_load_address_space(&g_kernel_address_space);
    heap_initialize(&g_kernel_address_space, ARCH_KHEAP_START, ARCH_KHEAP_END);

    gdt_initialize();

    if(!cpuid_feature(CPUID_FEAT_MSR)) panic("ARCH/AMD64", "MSRS are not supported on this system");

    acpi_initialize(boot_info->acpi_rsdp);
    acpi_fadt_t *fadt = (acpi_fadt_t *) acpi_find_table((uint8_t *) "FACP");

    interrupt_initialize();
    for(int i = 0; i < 32; i++) {
        interrupt_set(i, INTERRUPT_PRIORITY_EXCEPTION, panic_exception);
    }
    interrupt_load_idt();

    int sched_vector = interrupt_request(INTERRUPT_PRIORITY_KERNHIGH, sched_entry);
    if(sched_vector < 0) panic("ARCH/AMD64", "Unable to aquire an interrupt vector for the scheduler");
    g_sched_vector = sched_vector;

    pic8259_remap();
    if(!cpuid_feature(CPUID_FEAT_APIC)) {
        g_interrupt_irq_eoi = pic8259_eoi;
        panic("ARCH/AMD64", "Legacy PIC is not supported at the moment");
    } else {
        pic8259_disable();
        lapic_initialize();
        g_interrupt_irq_eoi = lapic_eoi;
    }

    acpi_sdt_header_t *madt = acpi_find_table((uint8_t *) "APIC");
    if(madt) ioapic_initialize(madt);

    common_init(&g_ctx);

    if(fadt && (acpi_revision() == 0 || (fadt->boot_architecture_flags & (1 << 1)))) {
        ps2kb_set_handler((ps2kb_handler_t) test_kb);
        ps2_initialize();
    }

    for(int i = 1; i <= 10; i++) {
        sched_thread_t *test_thread = heap_alloc(sizeof(sched_thread_t));
        memset(test_thread, 0, sizeof(sched_thread_t));
        test_thread->this = test_thread;
        test_thread->id = i;
        test_thread->context.registers.cs = GDT_CODE_RING0;
        test_thread->context.registers.ss = GDT_DATA_RING0;
        test_thread->context.registers.rflags = (1 << 9) | (1 << 1);
        test_thread->context.registers.rsp = HHDM(pmm_page_alloc(PMM_PAGE_USAGE_WIRED)->paddr) + ARCH_PAGE_SIZE;
        test_thread->context.registers.rip = (uint64_t) test_thread_loop;
        test_thread->address_space = &g_kernel_address_space;
        sched_add(test_thread);
    }

    g_cpus_initialized = 0;
    for(int i = 0; i < boot_info->cpu_count; i++) {
        if(i == boot_info->bsp_index) {
            g_cpus_initialized++;
            continue;
        }
        *boot_info->cpus[i].wake_on_write = (uint64_t) init_ap;
        while(i >= g_cpus_initialized);
    }

    init_common();

    for(;;) asm volatile("hlt");
    __builtin_unreachable();
}