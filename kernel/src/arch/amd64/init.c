#include <tartarus.h>
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
#include <arch/amd64/pic8259.h>
#include <arch/amd64/apic.h>
#include <arch/amd64/interrupt.h>
#include <arch/amd64/drivers/ioapic.h>
#include <arch/amd64/drivers/ps2.h>
#include <arch/amd64/drivers/ps2kb.h>
#include <graphics/draw.h>
#include <graphics/basicfont.h>

uintptr_t g_hhdm_address;

static draw_context_t g_ctx;
static int g_x = 10, g_y = 10;
int putchar(int c) {
    switch(c) {
        case '\n':
            g_x = 10;
            g_y += BASICFONT_HEIGHT;
            break;
        default:
            draw_char(&g_ctx, g_x, g_y, (char) c, 0xFFFFFF);
            g_x += BASICFONT_WIDTH;
            break;
    }
    return (char) c;
}

static void test_kb(uint8_t c) {
    putchar(c);
}

[[noreturn]] extern void kinit(tartarus_boot_info_t *boot_info) {
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

    uint64_t sp;
    asm volatile("mov %%rsp, %0" : "=rm" (sp));
    asm volatile("mov %0, %%rsp" : : "rm" (HHDM(sp)));
    uint64_t bp;
    asm volatile("mov %%rbp, %0" : "=rm" (bp));
    asm volatile("mov %0, %%rbp" : : "rm" (HHDM(bp)));

    vmm_address_space_t kernel_address_space;
    vmm_create_kernel_address_space(&kernel_address_space);
    arch_vmm_load_address_space(&kernel_address_space);
    heap_initialize(&kernel_address_space, ARCH_KHEAP_START, ARCH_KHEAP_END);

    gdt_initialize();

    if(!cpuid_feature(CPUID_FEAT_MSR)) panic("ARCH/AMD64", "MSRS are not supported on this system");

    uint64_t pat = msr_read(MSR_PAT);
    pat &= ~(((uint64_t) 0b111 << 48) | ((uint64_t) 0b111 << 40));
    pat |= ((uint64_t) 0x1 << 48) | ((uint64_t) 0x5 << 40);
    msr_write(MSR_PAT, pat);

    acpi_initialize(boot_info->acpi_rsdp);
    acpi_fadt_t *fadt = (acpi_fadt_t *) acpi_find_table((uint8_t *) "FACP");

    interrupt_initialize();
    pic8259_remap();
    if(!cpuid_feature(CPUID_FEAT_APIC)) {
        g_interrupt_irq_eoi = pic8259_eoi;
        panic("ARCH/AMD64", "Legacy PIC is not supported at the moment");
    } else {
        pic8259_disable();
        apic_initialize();
        g_interrupt_irq_eoi = apic_eoi;
    }
    for(int i = 0; i < 32; i++) {
        interrupt_set(i, INTERRUPT_PRIORITY_EXCEPTION, panic_exception);
    }

    acpi_sdt_header_t *madt = acpi_find_table((uint8_t *) "APIC");
    if(madt) ioapic_initialize(madt);

    asm volatile("sti");

    common_init(&g_ctx);

    if(fadt && (acpi_revision() == 0 || (fadt->boot_architecture_flags & (1 << 1)))) {
        ps2kb_set_handler((ps2kb_handler_t) test_kb);
        ps2_initialize();
    }

    for(;;) asm volatile("hlt");
    __builtin_unreachable();
}