#include <stdnoreturn.h>
#include <stdbool.h>
#include <tartarus.h>
#include <stdio.h>
#include <string.h>
#include <panic.h>
#include <memory/hhdm.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <memory/heap.h>
#include <graphics/basicfont.h>
#include <graphics/draw.h>
#include <drivers/acpi.h>
#include <cpu/pic8259.h>
#include <cpu/apic.h>
#include <cpu/exceptions.h>
#include <cpu/irq.h>
#include <cpu/idt.h>
#include <cpu/gdt.h>
#include <cpu/msr.h>
#include <drivers/pit.h>
#include <drivers/keyboard.h>
#include <drivers/mouse.h>
#include <drivers/ps2.h>
#include <drivers/pci.h>
#include <drivers/ahci.h>
#include <drivers/hpet.h>
#include <fs/vfs.h>
#include <fs/fat32.h>
#include <userspace/syscall.h>
#include <proc/sched.h>
#include <kcon.h>
#include <kdesktop.h>

#define PAGE_SIZE 0x1000

static draw_colormask_t g_fb_colormask;
static draw_context_t g_fb_context;

extern noreturn void kmain(tartarus_parameters_t *boot_params) {
    g_hhdm_address = boot_params->hhdm_address;

    g_fb_colormask = (draw_colormask_t) {
        .red_size = boot_params->framebuffer->mask_red_size,
        .red_shift = boot_params->framebuffer->mask_red_shift,
        .green_size = boot_params->framebuffer->mask_green_size,
        .green_shift = boot_params->framebuffer->mask_green_shift,
        .blue_size = boot_params->framebuffer->mask_blue_size,
        .blue_shift = boot_params->framebuffer->mask_blue_shift
    };
    g_fb_context = (draw_context_t) {
        .width = boot_params->framebuffer->width,
        .height = boot_params->framebuffer->height,
        .pitch = boot_params->framebuffer->pitch / 4,
        .address = (void *) HHDM(boot_params->framebuffer->address),
        .colormask = &g_fb_colormask,
        .invalidated = true
    };

    uint16_t memap_length = boot_params->memory_map_length;
    tartarus_memap_entry_t *memap = (tartarus_memap_entry_t *) HHDM(boot_params->memory_map);

    // TODO: MSR Available WTF to do if its not ig
    if(!msr_available()) panic("KERNEL", "MSRS are not available");

    gdt_initialize();
    pmm_initialize(boot_params->memory_map, boot_params->memory_map_length);

    uint64_t sp;
    asm volatile("mov %%rsp, %0" : "=rm" (sp));
    asm volatile("mov %0, %%rsp" : : "rm" (HHDM(sp)));
    uint64_t bp;
    asm volatile("mov %%rbp, %0" : "=rm" (bp));
    asm volatile("mov %0, %%rbp" : : "rm" (HHDM(bp)));

    uint64_t pml4;
    asm volatile("mov %%cr3, %0" : "=r" (pml4));
    vmm_initialize(pml4);
    heap_initialize((void *) 0xFFFFFF0000000000, 10);

    acpi_initialize();
    acpi_fadt_t *fadt = (acpi_fadt_t *) acpi_find_table((uint8_t *) "FACP");

    pic8259_remap();
    exceptions_initialize();
    irq_initialize();
    // TODO: APIC address needs to be retrieved from an MSR rather than acpi tables
    acpi_sdt_header_t *apic_header = acpi_find_table((uint8_t *) "APIC");
    if(apic_header) {
        pic8259_disable();
        apic_initialize(apic_header);
    }
    idt_initialize();
    asm volatile("sti");

    acpi_sdt_header_t *mcfg_header = acpi_find_table((uint8_t *) "MCFG");
    if(mcfg_header) {
        pci_express_enumerate(mcfg_header);
    } else {
        pci_enumerate();
    }

    pit_initialize();

    kcon_initialize(&g_fb_context);
    keyboard_set_handler(kcon_keyboard_handler);

    acpi_sdt_header_t *hpet_header = acpi_find_table((uint8_t *) "HPET");
    if(hpet_header) {
        hpet_initialize();
    }

    if(fadt && (acpi_revision() == 0 || (fadt->boot_architecture_flags & (1 << 1)))) {
        ps2_initialize();
    }

    gdt_tss_initialize();

    // TODO: Do we just want to panic or do we want an alternative way of implementing syscalls
    if(!syscall_available()) panic("KERNEL", "Syscalls not available");
    syscall_initialize();

    sched_handoff();
    __builtin_unreachable();
}

noreturn void panic(char *location, char *msg) {
    draw_color_t bg = draw_color(&g_fb_context, 255, 60, 60);
    draw_rect(&g_fb_context, 0, 0, g_fb_context.width, g_fb_context.height, bg);
    int y = (g_fb_context.height - 3 * BASICFONT_HEIGHT) / 2;
    char *title = "KERNEL PANIC";
    draw_string_simple(&g_fb_context, (g_fb_context.width - strlen(title) * BASICFONT_WIDTH) / 2, y, title, 0xFFFFFFFF);
    draw_string_simple(&g_fb_context, (g_fb_context.width - strlen(location) * BASICFONT_WIDTH) / 2, y + BASICFONT_HEIGHT, location, 0xFFFFFFFF);
    draw_string_simple(&g_fb_context, (g_fb_context.width - strlen(msg) * BASICFONT_WIDTH) / 2, y + BASICFONT_HEIGHT * 2, msg, 0xFFFFFFFF);

    asm volatile("cli");
    asm volatile("hlt");
    __builtin_unreachable();
}

static void panic_prntnum(draw_context_t *ctx, uint16_t x, uint16_t y, uint64_t value) {
    int cc = 0;
    uint64_t pw = 1;
    while(value / pw >= 16) pw *= 16;

    draw_char(ctx, x + BASICFONT_WIDTH * cc++, y, '0', 0xFFFFFFFF);
    draw_char(ctx, x + BASICFONT_WIDTH * cc++, y, 'x', 0xFFFFFFFF);
    while(pw > 0) {
        uint8_t c = value / pw;
        if(c >= 10) {
            draw_char(ctx, x + BASICFONT_WIDTH * cc++, y, c - 10 + 'a', 0xFFFFFFFF);
        } else {
            draw_char(ctx, x + BASICFONT_WIDTH * cc++, y, c + '0', 0xFFFFFFFF);
        }
        value %= pw;
        pw /= 16;
    }
}

noreturn void panic_exception(char *msg, exception_frame_t regs) {
    draw_color_t bg = draw_color(&g_fb_context, 255, 60, 60);
    draw_rect(&g_fb_context, 0, 0, g_fb_context.width, g_fb_context.height, bg);
    int y = (g_fb_context.height - 3 * BASICFONT_HEIGHT) / 2;
    char *title = "EXCEPTION";
    draw_string_simple(&g_fb_context, (g_fb_context.width - strlen(title) * BASICFONT_WIDTH) / 2, y, title, 0xFFFFFFFF);
    draw_string_simple(&g_fb_context, (g_fb_context.width - strlen(msg) * BASICFONT_WIDTH) / 2, y + BASICFONT_HEIGHT, msg, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, g_fb_context.width / 2, y + BASICFONT_HEIGHT * 2, (uint64_t) regs.err_code);
    panic_prntnum(&g_fb_context, g_fb_context.width / 2, y + BASICFONT_HEIGHT * 3, regs.rip);

    asm volatile("cli");
    asm volatile("hlt");
    __builtin_unreachable();
}