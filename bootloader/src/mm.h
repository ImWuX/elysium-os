#ifndef MM_H
#define MM_H

#include <stdint.h>
#include <boot/memap.h>

typedef struct {
    uint64_t address;
    uint64_t length;
    uint32_t type;
    uint32_t acpi3attr;
} __attribute__((packed)) e820_entry_t;

typedef enum {
    E820_TYPE_USABLE = 1,
    E820_TYPE_RESERVED,
    E820_TYPE_ACPI_RECLAIMABLE,
    E820_TYPE_ACPI_NVS,
    E820_TYPE_BAD,
} e820_type_t;

extern boot_memap_entry_t *g_memap;
extern uint64_t g_memap_length;

void mm_initialize();
void *mm_request_page();
void *mm_request_linear_pages(uint64_t number_of_pages);
void *mm_request_linear_pages_type(uint64_t number_of_pages, boot_memap_entry_type_t type);

#endif