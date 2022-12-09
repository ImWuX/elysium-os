#include <stdnoreturn.h>
#include <stdint.h>
#include <stdbool.h>
#include <mm.h>
#include <paging.h>
#include <bootlog.h>
#include <pci.h>
#include <fat32.h>
#include <elf.h>
#include <boot/params.h>

#define KERNEL_FILE "KERNEL  SYS"
#define BOOT_DRIVE_ADDRESS 0x7C40

extern void* ld_vbe_mode_info;
extern void* ld_mmap;

extern noreturn void bootmain() {
    boot_log_clear();
    boot_log("Welcome to the 64bit NestOS bootloader\n", LOG_LEVEL_INFO);

    mm_initialize();
    paging_initialize(g_memap, g_memap_length);

    boot_parameters_t *boot_params = (boot_parameters_t *) mm_request_page();
    boot_params->boot_drive = *((uint8_t *) BOOT_DRIVE_ADDRESS);
    boot_params->vbe_mode_info_address = (uint64_t) &ld_vbe_mode_info;

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
            boot_log("Bootloader finished\n", LOG_LEVEL_INFO);

            for(uint64_t i = 0; i < g_memap_length; i++) {
                boot_log("Type[", LOG_LEVEL_INFO);
                boot_log_hex(g_memap[i].type);
                boot_log("] Base[", LOG_LEVEL_INFO);
                boot_log_hex(g_memap[i].base_address);
                boot_log("] Length[", LOG_LEVEL_INFO);
                boot_log_hex(g_memap[i].length);
                boot_log("]\n", LOG_LEVEL_INFO);
            }

            // void (*kmain)(boot_parameters_t *) = (void (*)()) entry;
            // kmain(boot_params);
            boot_log("Kernel exited\n", LOG_LEVEL_ERROR);
            asm("cli");
            asm("hlt");
        }
        directory = directory->last_entry;
    }

    boot_log("Bootloader failed to load kernel\n", LOG_LEVEL_ERROR);
    while(true) asm("hlt");

    // TODO: Both AHCI driver and FAT32 driver assumes sector size.. ig mbr does as well but we should support it atleast in the C code
}