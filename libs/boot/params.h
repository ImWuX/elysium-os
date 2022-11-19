#ifndef BOOT_BOOTPARAMS_H
#define BOOT_BOOTPARAMS_H

typedef struct {
    uint8_t boot_drive;
    uint64_t bios_memory_map_address;
    uint64_t memory_map_buffer_address; //TODO: Bootloader should have a better way of passing memory map
    uint64_t memory_map_buffer_size;
    uint64_t memory_map_free_mem;
    uint64_t memory_map_reserved_mem;
    uint64_t memory_map_used_mem;

    uint64_t paging_address; //TODO: Again bootloader should better prepare the kernel

    uint64_t vbe_mode_info_address;
} __attribute__((packed)) boot_parameters_t;

#endif