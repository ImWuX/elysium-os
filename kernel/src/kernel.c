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
#include <fs/tmpfs.h>
#include <userspace/syscall.h>
#include <proc/sched.h>
#include <kcon.h>
#include <kdesktop.h>

static draw_colormask_t g_fb_colormask;
static draw_context_t g_fb_context;

noreturn void exception_handler(interrupt_frame_t *regs);

extern noreturn void kmain(tartarus_parameters_t *boot_params) {
    g_hhdm_address = boot_params->hhdm_start;

    g_fb_context = (draw_context_t) {
        .width = boot_params->framebuffer->width,
        .height = boot_params->framebuffer->height,
        .pitch = boot_params->framebuffer->pitch / 4,
        .address = (void *) HHDM(boot_params->framebuffer->address),
        .invalidated = true
    };

    void *rsdp = boot_params->rsdp;

    panic_initialize(&g_fb_context, (char *) HHDM(boot_params->kernel_symbols), boot_params->kernel_symbols_size);

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

    vfs_initialize(tmpfs_create());
    vfs_node_t *root;
    g_vfs->ops->root(g_vfs, &root);
    vfs_node_t *home;
    vfs_node_attributes_t attributes = { .type = VFS_NODE_TYPE_DIRECTORY };
    root->ops->create(root, &home, "home", &attributes);
    kcon_initialize_fs(home);

    gdt_tss_initialize();

    // TODO: Do we just want to panic or do we want an alternative way of implementing syscalls
    if(!syscall_available()) panic("KERNEL", "Syscalls not available");
    syscall_initialize();

    sched_handoff();
    __builtin_unreachable();
}

void print_tmpfs(tmpfs_node_t *node, int level) {
    for(int i = 0; i < level * 2; i++) printf(" ");
    printf("%s\n", node->name);
    if(node->is_dir) {
        node = node->children;
        while(node) {
            print_tmpfs(node, level + 1);
            node = node->next;
        }
    }
}