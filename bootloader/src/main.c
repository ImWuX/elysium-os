#include <stdnoreturn.h>
#include <stdint.h>
#include <stdbool.h>
#include <memory.h>
#include <paging.h>
#include <bootlog.h>
#include <pci.h>
#include <fat32.h>
#include <elf.h>
#include <boot/bootparams.h>

#define KERNEL_FILE "KERNEL  SYS"

extern void* ld_vbe_mode_info;
extern void* ld_mmap;

extern noreturn void bootmain() {
    boot_log("Long mode enabled.", LOG_LEVEL_INFO);

    initialize_paging();
    initialize_memory();

    boot_parameters_t *boot_params = (boot_parameters_t *) request_page();
    boot_params->boot_drive = *((uint8_t *) 0x7C40);
    boot_params->memory_map_buffer_address = get_buffer_address();
    boot_params->memory_map_buffer_size = get_buffer_size();
    boot_params->memory_map_free_mem = get_free_memory();
    boot_params->memory_map_reserved_mem = get_reserved_memory();
    boot_params->memory_map_used_mem = get_used_memory();
    boot_params->paging_address = get_pml4_address();
    boot_params->vbe_mode_info_address = (uint64_t) &ld_vbe_mode_info;
    boot_params->bios_memory_map_address = (uint64_t) &ld_mmap;

    pci_enumerate();
    initialize_fs();

    char *kernel_name = KERNEL_FILE;
    dir_entry_t *directory = read_root_directory();
    while(directory != 0) {
        bool is_kernel = true;
        for(int i = 0; i < 11; i++) {
            if(directory->fd.name[i] != (uint8_t) kernel_name[i]) {
                is_kernel = false;
                break;
            }
        }
        if(is_kernel) {
            elf64_addr_t entry = read_elf_file(&directory->fd);
            if(entry == 0) break;
            boot_log("Bootloader finished", LOG_LEVEL_INFO);
            void (*kmain)(boot_parameters_t *) = (void (*)()) entry;
            kmain(boot_params);
            while(1) asm("hlt");
        }
        directory = directory->last_entry;
    }

    boot_log("Bootloader failed to load kernel", LOG_LEVEL_ERROR);
    while(1) asm("hlt");

    // TODO: Both AHCI driver and FAT32 driver assumes sector size.. ig mbr does as well but we should support it atleast in the C code
}