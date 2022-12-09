#ifndef LIBS_BOOT_PARAMS_H
#define LIBS_BOOT_PARAMS_H

#include "memap.h"

typedef struct {
    uint8_t boot_drive;
    boot_memap_entry_t *memory_map;
    uint64_t memory_map_length;
    uint64_t vbe_mode_info_address;
} __attribute__((packed)) boot_parameters_t;

#endif