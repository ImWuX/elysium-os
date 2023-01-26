#include <stdnoreturn.h>
#include <stdbool.h>
#include <tartarus.h>
#include <stdio.h>
#include <panic.h>
#include <memory/hhdm.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <memory/heap.h>
#include <graphics/basicfont.h>
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

uint64_t g_hhdm_address;

static tartarus_framebuffer_t g_framebuffer;
static uint16_t g_x;
static uint16_t g_y;

int putchar(int c) {
    switch(c) {
        case '\n':
            g_x = 0;
            g_y += BASICFONT_HEIGHT;
            break;
        case '\t':
            g_x += (4 - (g_x / BASICFONT_WIDTH) % 4) * BASICFONT_WIDTH;
            break;
        default:
            uint8_t *font_char = &g_basicfont[c * 16];

            uint32_t *buf = (uint32_t *) HHDM(g_framebuffer.address);
            int offset = g_x + g_y * g_framebuffer.pitch / 4;
            for(int xx = 0; xx < BASICFONT_HEIGHT; xx++) {
                for(int yy = 0; yy < BASICFONT_WIDTH; yy++) {
                    if(font_char[xx] & (1 << (BASICFONT_WIDTH - yy))){
                        buf[offset + yy] = 0xFFFFFFFF;
                    } else {
                        buf[offset + yy] = 0;
                    }
                }
                offset += g_framebuffer.pitch / 4;
            }
            g_x += BASICFONT_WIDTH;
            break;
    }
    if(g_x >= g_framebuffer.width) {
        g_x = 0;
        g_y += BASICFONT_HEIGHT;
    }
    if(g_y >= g_framebuffer.height) g_y = g_framebuffer.height - BASICFONT_HEIGHT - 1;
    return c;
}

static void keyboard_handler(uint8_t character) {
    putchar(character);
}

extern noreturn void kmain(tartarus_parameters_t *boot_params) {
    g_hhdm_address = boot_params->hhdm_address;
    g_framebuffer = *boot_params->framebuffer;

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

    pit_initialize();
    keyboard_initialize();
    keyboard_set_handler(keyboard_handler);

    while(true) asm volatile("hlt");
    __builtin_unreachable();
}

noreturn void panic(char *location, char *msg) {
    printf(">> Kernel Panic [%s] %s", location, msg);
    asm volatile("cli");
    asm volatile("hlt");
    __builtin_unreachable();
}