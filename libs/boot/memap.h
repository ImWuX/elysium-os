#ifndef LIBS_BOOT_MEMAP_H
#define LIBS_BOOT_MEMAP_H

#include <stdint.h>

typedef enum {
    BOOT_MEMAP_TYPE_USABLE = 0,
    BOOT_MEMAP_TYPE_BOOT_RECLAIMABLE,
    BOOT_MEMAP_TYPE_ACPI_RECLAIMABLE,
    BOOT_MEMAP_TYPE_ACPI_NVS,
    BOOT_MEMAP_TYPE_KERNEL,
    BOOT_MEMAP_TYPE_RESERVED,
    BOOT_MEMAP_TYPE_BAD,
} boot_memap_entry_type_t;

typedef struct {
    uint64_t base_address;
    uint64_t length;
    boot_memap_entry_type_t type;
} boot_memap_entry_t;

#endif