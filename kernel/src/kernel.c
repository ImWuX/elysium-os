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
#include <drivers/pci.h>
#include <drivers/ahci.h>
#include <fs/vfs.h>
#include <fs/fat32.h>
#include <kcon.h>

extern noreturn void kmain(tartarus_parameters_t *boot_params) {
    g_hhdm_address = boot_params->hhdm_address;

    draw_framebuffer_t framebuffer = {
        .width = boot_params->framebuffer->width,
        .height = boot_params->framebuffer->height,
        .bpp = boot_params->framebuffer->bpp,
        .pitch = boot_params->framebuffer->pitch,
        .address = boot_params->framebuffer->address
    };
    draw_colormask_t color_mask = {
        .red_size = boot_params->framebuffer->mask_red_size,
        .red_shift = boot_params->framebuffer->mask_red_shift,
        .green_size = boot_params->framebuffer->mask_green_size,
        .green_shift = boot_params->framebuffer->mask_green_shift,
        .blue_size = boot_params->framebuffer->mask_blue_size,
        .blue_shift = boot_params->framebuffer->mask_blue_shift
    };
    draw_initialize(color_mask, framebuffer);
    kcon_initialize(1000, 600, (boot_params->framebuffer->width - 1000) / 2, (boot_params->framebuffer->height - 600) / 2);

    printf(" _____ _         _           _____ _____ \n");
    printf("|   __| |_ _ ___|_|_ _ _____|     |   __|\n");
    printf("|   __| | | |_ -| | | |     |  |  |__   |\n");
    printf("|_____|_|_  |___|_|___|_|_|_|_____|_____|\n");
    printf("        |___|                            \n");
    printf("\n");
    printf("Welcome to Elysium OS\n");

    gdt_initialize();

    pmm_initialize(boot_params->memory_map, boot_params->memory_map_length);
    printf("Physical Memory Initialized\n");
    printf("Stats:\n\tTotal: %i bytes\n\tFree: %i bytes\n\tUsed: %i bytes\n", pmm_mem_total(), pmm_mem_free(), pmm_mem_used());

    uint64_t sp;
    asm volatile("mov %%rsp, %0" : "=rm" (sp));
    asm volatile("mov %0, %%rsp" : : "rm" (HHDM(sp)));
    uint64_t bp;
    asm volatile("mov %%rbp, %0" : "=rm" (bp));
    asm volatile("mov %0, %%rbp" : : "rm" (HHDM(bp)));

    uint64_t pml4;
    asm volatile("mov %%cr3, %0" : "=r" (pml4));
    vmm_initialize(pml4);
    printf("Virtual Memory Initialized\n");

    heap_initialize((void *) 0x100000000000, 10);
    printf("Heap Initialized\n");

    acpi_initialize();
    printf("ACPI Initialized\n");

    pic8259_remap();
    exceptions_initialize();
    irq_initialize();
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
    keyboard_initialize();
    keyboard_set_handler(kcon_keyboard_handler);
    kcon_print_prefix();

    // fat32_initialize();

    while(true) asm volatile("hlt");
    __builtin_unreachable();
}

noreturn void panic(char *location, char *msg) {
    draw_color_t bg = draw_color(255, 60, 60);
    draw_rect(0, 0, draw_scrw(), draw_scrh(), bg);
    int y = (draw_scrh() - 3 * BASICFONT_HEIGHT) / 2;
    char *title = "KERNEL PANIC";
    draw_string_simple((draw_scrw() - strlen(title) * BASICFONT_WIDTH) / 2, y, title, 0xFFFFFFFF, bg);
    draw_string_simple((draw_scrw() - strlen(location) * BASICFONT_WIDTH) / 2, y + BASICFONT_HEIGHT, location, 0xFFFFFFFF, bg);
    draw_string_simple((draw_scrw() - strlen(msg) * BASICFONT_WIDTH) / 2, y + BASICFONT_HEIGHT * 2, msg, 0xFFFFFFFF, bg);
    asm volatile("cli");
    asm volatile("hlt");
    __builtin_unreachable();
}