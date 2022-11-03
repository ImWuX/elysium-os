#ifndef MEMORY_PMM_H
#define MEMORY_PMM_H

#include <stdint.h>

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

void initialize_paging(uint64_t buf_address, uint64_t buf_size, uint64_t free, uint64_t reserved, uint64_t used);

void *request_page();
void *request_linear_pages(int count);
uint8_t page_state(void *address);
void lock_page(void *address);
void lock_pages(void *address, uint64_t count);
void reserve_page(void *address);
void reserve_pages(void *address, uint64_t count);
void free_page(void *address);
void free_pages(void *address, uint64_t count);

uint64_t get_free_memory();
uint64_t get_used_memory();
uint64_t get_reserved_memory();

#endif