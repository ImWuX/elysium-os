#include <tartarus.h>
#include <string.h>
#include <cpuid.h>
#include <panic.h>
#include <common.h>
#include <lib/kprint.h>
#include <memory/hhdm.h>
#include <memory/heap.h>
#include <drivers/acpi.h>
#include <arch/vmm.h>
#include <arch/sched.h>
#include <arch/amd64/vmm.h>
#include <arch/amd64/gdt.h>
#include <arch/amd64/msr.h>
#include <arch/amd64/cpuid.h>
#include <arch/amd64/lapic.h>
#include <arch/amd64/exception.h>
#include <arch/amd64/interrupt.h>
#include <arch/amd64/sched.h>
#include <arch/amd64/drivers/pic8259.h>
#include <arch/amd64/drivers/ioapic.h>
#include <arch/amd64/drivers/ps2.h>
#include <arch/amd64/drivers/ps2kb.h>
#include <arch/amd64/drivers/ps2mouse.h>
#include <arch/amd64/drivers/pit.h>
#include <graphics/draw.h>
#include <istyx.h>

#define LAPIC_CALIBRATION_TICKS 0x10000

uintptr_t g_hhdm_address;
vmm_address_space_t g_kernel_address_space;
static draw_context_t g_fb_context;
static volatile int g_cpus_initialized;

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
    thread->cpu_local = cpu_local;
    thread->state = SCHED_THREAD_EXIT;
    arch_sched_set_current_thread(thread);

    asm volatile("sti");

    lapic_timer_oneshot(g_sched_vector, 10'000);
}

[[noreturn]] __attribute__((naked)) void init_ap() {
    asm volatile("mov %%rsp, %%rax\nadd %0, %%rax\nmov %%rax, %%rsp" : : "r" (g_hhdm_address) : "rax", "memory");
    asm volatile("mov %%rbp, %%rax\nadd %0, %%rax\nmov %%rax, %%rbp" : : "r" (g_hhdm_address) : "rax", "memory");

    gdt_load();
    interrupt_load_idt();

    if(!cpuid_feature(CPUID_FEAT_APIC)) panic("ARCH/AMD64/INITAP", "Non-APIC CPUs are not supported");
    lapic_initialize();

    init_common();

    g_cpus_initialized++;
    for(;;) asm volatile("hlt");
    __builtin_unreachable();
}

[[noreturn]] __attribute__((naked)) extern void init(tartarus_boot_info_t *boot_info) {
    g_fb_context.address = boot_info->framebuffer.address;
    g_fb_context.width = boot_info->framebuffer.width;
    g_fb_context.height = boot_info->framebuffer.height;
    g_fb_context.pitch = boot_info->framebuffer.pitch;
    istyx_early_initialize(&g_fb_context);

    if(boot_info->hhdm_base < ARCH_HHDM_START || boot_info->hhdm_base + boot_info->hhdm_size >= ARCH_HHDM_END) panic("KERNEL", "HHDM is not within arch specific boundaries");
    g_hhdm_address = boot_info->hhdm_base;

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

    gdt_load();

    if(!cpuid_feature(CPUID_FEAT_MSR)) panic("ARCH/AMD64", "MSRS are not supported on this system");

    interrupt_initialize();
    for(int i = 0; i < 32; i++) {
        interrupt_set(i, INTERRUPT_PRIORITY_EXCEPTION, exception_unhandled);
    }
    interrupt_load_idt();

    int sched_vector = interrupt_request(INTERRUPT_PRIORITY_KERNHIGH, sched_entry);
    if(sched_vector < 0) panic("ARCH/AMD64", "Unable to acquire an interrupt vector for the scheduler");
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

    common_init(boot_info->acpi_rsdp);

    acpi_sdt_header_t *madt = acpi_find_table((uint8_t *) "APIC");
    if(madt) ioapic_initialize(madt);

    acpi_fadt_t *fadt = (acpi_fadt_t *) acpi_find_table((uint8_t *) "FACP");
    if(fadt && (acpi_revision() == 0 || (fadt->boot_architecture_flags & (1 << 1)))) {
        ps2kb_set_handler((ps2kb_handler_t) istyx_simple_input_kb);
        ps2mouse_set_handler((ps2mouse_handler_t) istyx_simple_input_mouse);
        ps2_initialize();
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