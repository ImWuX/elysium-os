#include <stdnoreturn.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <boot/params.h>
#include <memory/pmm.h>
#include <memory/vmm.h>
#include <memory/heap.h>
#include <cpu/exceptions.h>
#include <cpu/irq.h>
#include <cpu/idt.h>
#include <cpu/pit.h>
#include <cpu/cpuid.h>
#include <cpu/apic.h>
#include <hal/pic8259.h>
#include <drivers/display.h>
#include <drivers/acpi.h>
#include <drivers/pci.h>
#include <fs/fat32.h>
#include <user/mouse.h>
#include <user/keyboard.h>
#include <user/kconsole.h>

extern noreturn void kmain(boot_parameters_t *boot_params) {
    // Initialize memory management
    // initialize_paging(
    //     boot_params->memory_map_buffer_address,
    //     boot_params->memory_map_buffer_size,
    //     boot_params->memory_map_free_mem,
    //     boot_params->memory_map_reserved_mem,
    //     boot_params->memory_map_used_mem
    // );
    // initialize_memory(boot_params->paging_address);
    initialize_heap((void *) 0x100000000000, 10);

    // Initialize ACPI
    initialize_acpi();

    // Initialize interrupts
    pic8259_remap();
    initialize_exceptions();
    initialize_irqs();

    sdt_header_t *apic_header = acpi_find_table((uint8_t *) "APIC");
    if(cpuid_check_capability(CPUID_CAPABILITY_EDX_APIC) && apic_header) {
        pic8259_disable();
        initialize_apic(apic_header);
    }
    initialize_idt();
    asm volatile("sti");
    initialize_timer();

    // Initialize graphics
    initialize_display(boot_params->vbe_mode_info_address);
    initialize_kconsole(200, 200, get_display_mode_info()->width - 400, get_display_mode_info()->height - 400);

    // Initialize drivers
    sdt_header_t *mcfg_header = acpi_find_table((uint8_t *) "MCFG");
    if(mcfg_header) {
        pci_express_enumerate(mcfg_header);
    } else {
        pci_enumerate();
    }

    initialize_fs();

    // Initialize user environment
    initialize_mouse();
    initialize_keyboard();
    set_keyboard_handler(keyboard_handler);

    while(true) asm volatile("hlt");
}