#ifndef DRIVERS_HPET_H
#define DRIVERS_HPET_H

#include <stdint.h>
#include <drivers/acpi.h>

typedef struct {
    uint8_t address_space_id;
    uint8_t register_bit_width;
    uint8_t register_bit_offset;
    uint8_t reserved;
    uint64_t address;
} __attribute__((packed)) hpet_address_structure;

typedef struct {
    acpi_sdt_header_t sdt_header;
    uint32_t event_timer_block_id;
    hpet_address_structure address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} __attribute__((packed)) hpet_header_t;

void hpet_initialize();

#endif