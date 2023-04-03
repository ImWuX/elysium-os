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
#include <drivers/ioapic.h>
#include <cpu/pic8259.h>
#include <cpu/apic.h>
#include <cpu/interrupt.h>
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

static draw_colormask_t g_fb_colormask;
static draw_context_t g_fb_context;

noreturn void exception_handler(interrupt_frame_t *regs);

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

    void *rsdp = boot_params->rsdp;

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

    kcon_initialize(&g_fb_context);
    keyboard_set_handler(kcon_keyboard_handler);

    if(!rsdp) panic("KERNEL", "No RSDP found");
    acpi_initialize((acpi_rsdp_t *) HHDM(rsdp));
    acpi_fadt_t *fadt = (acpi_fadt_t *) acpi_find_table((uint8_t *) "FACP");

    pic8259_remap();
    acpi_sdt_header_t *apic_header = acpi_find_table((uint8_t *) "APIC");
    if(apic_header) {
        pic8259_disable();
        apic_initialize();
        ioapic_initialize(apic_header);
        g_interrupt_irq_eoi = apic_eoi;
    } else {
        g_interrupt_irq_eoi = pic8259_eoi;
        panic("KERNEL", "Legacy 8529pic is currently unsupported.");
    }
    interrupt_initialize();
    for(int i = 0; i < 32; i++) {
        interrupt_set(i, INTERRUPT_PRIORITY_EXCEPTION, exception_handler);
    }
    asm volatile("sti");

    pci_enumerate(acpi_find_table((uint8_t *) "MCFG"));

    pit_initialize();
    acpi_sdt_header_t *hpet_header = acpi_find_table((uint8_t *) "HPET");
    if(hpet_header) hpet_initialize(hpet_header);

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

static char *g_exception_messages[] = {
    "Division by Zero",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bat TSS",
    "Segment not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

noreturn void exception_handler(interrupt_frame_t *regs) {
    draw_color_t bg = draw_color(&g_fb_context, 255, 60, 60);
    draw_rect(&g_fb_context, 0, 0, g_fb_context.width, g_fb_context.height, bg);

    char *title = "EXCEPTION";
    char *r15 = "r15: ";
    char *r14 = "r14: ";
    char *r13 = "r13: ";
    char *r12 = "r12: ";
    char *r11 = "r11: ";
    char *r10 = "r10: ";
    char *r9 = "r9: ";
    char *r8 = "r8: ";
    char *rdi = "rdi: ";
    char *rsi = "rsi: ";
    char *rbp = "rbp: ";
    char *rsp = "rsp: ";
    char *rdx = "rdx: ";
    char *rcx = "rcx: ";
    char *rbx = "rbx: ";
    char *rax = "rax: ";
    char *int_no = "int_no: ";
    char *err_code = "err_code: ";
    char *cr2 = "cr2: ";
    char *rip = "rip: ";
    char *cs = "cs: ";
    char *rflags = "rflags: ";
    char *userrsp = "userrsp: ";
    char *ss = "ss: ";
    int x = g_fb_context.width / 3;
    int y = g_fb_context.height / 10;
    draw_string_simple(&g_fb_context, x, y, title, 0xFFFFFFFF);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, g_exception_messages[regs->int_no], 0xFFFFFFFF);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, r15, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(r15) * BASICFONT_WIDTH, y, (uint64_t) regs->r15);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, r14, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(r14) * BASICFONT_WIDTH, y, (uint64_t) regs->r14);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, r13, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(r13) * BASICFONT_WIDTH, y, (uint64_t) regs->r13);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, r12, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(r12) * BASICFONT_WIDTH, y, (uint64_t) regs->r12);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, r11, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(r11) * BASICFONT_WIDTH, y, (uint64_t) regs->r11);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, r10, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(r10) * BASICFONT_WIDTH, y, (uint64_t) regs->r10);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, r9, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(r9) * BASICFONT_WIDTH, y, (uint64_t) regs->r9);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, r8, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(r8) * BASICFONT_WIDTH, y, (uint64_t) regs->r8);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, rdi, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(rdi) * BASICFONT_WIDTH, y, (uint64_t) regs->rdi);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, rsi, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(rsi) * BASICFONT_WIDTH, y, (uint64_t) regs->rsi);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, rbp, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(rbp) * BASICFONT_WIDTH, y, (uint64_t) regs->rbp);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, rdx, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(rdx) * BASICFONT_WIDTH, y, (uint64_t) regs->rdx);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, rcx, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(rcx) * BASICFONT_WIDTH, y, (uint64_t) regs->rcx);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, rbx, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(rbx) * BASICFONT_WIDTH, y, (uint64_t) regs->rbx);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, rax, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(rax) * BASICFONT_WIDTH, y, (uint64_t) regs->rax);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, int_no, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(int_no) * BASICFONT_WIDTH, y, (uint64_t) regs->int_no);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, err_code, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(err_code) * BASICFONT_WIDTH, y, (uint64_t) regs->err_code);
    y += BASICFONT_HEIGHT;
    uint64_t cr2_value;
    asm volatile("movq %%cr2, %0" : "=r" (cr2_value));
    draw_string_simple(&g_fb_context, x, y, cr2, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(cr2) * BASICFONT_WIDTH, y, (uint64_t) cr2_value);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, rip, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(rip) * BASICFONT_WIDTH, y, (uint64_t) regs->rip);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, cs, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(cs) * BASICFONT_WIDTH, y, (uint64_t) regs->cs);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, rflags, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(rflags) * BASICFONT_WIDTH, y, (uint64_t) regs->rflags);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, userrsp, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(userrsp) * BASICFONT_WIDTH, y, (uint64_t) regs->userrsp);
    y += BASICFONT_HEIGHT;
    draw_string_simple(&g_fb_context, x, y, ss, 0xFFFFFFFF);
    panic_prntnum(&g_fb_context, x + strlen(ss) * BASICFONT_WIDTH, y, (uint64_t) regs->ss);

    asm volatile("cli\nhlt");
    __builtin_unreachable();
}