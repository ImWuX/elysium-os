#ifndef MEMORY_PAGING_H
#define MEMORY_PAGING_H

#include <stdint.h>

typedef struct {
    uint64_t address;
    uint64_t length;
    uint32_t type;
    uint32_t acpi3attr;
} __attribute__((packed)) e820_entry_t;

typedef enum {
    E820_TYPE_USABLE = 1,
    E820_TYPE_RESERVED = 2,
    E820_TYPE_ACPI_RECLAIMABLE = 3,
    E820_TYPE_ACPI_NVS = 4,
    E820_TYPE_BAD = 5,
} e820_type_t;

void initialize_paging();

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
uint64_t get_buffer_address();
uint64_t get_buffer_size();

#endif