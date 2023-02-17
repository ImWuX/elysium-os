#ifndef DRIVERS_HPET_H
#define DRIVERS_HPET_H

#include <stdint.h>
#include <stdbool.h>
#include <drivers/acpi.h>

typedef struct {
    acpi_sdt_header_t sdt_header;
    uint32_t event_timer_block_id;
    acpi_generic_address_structure_t address;
    uint8_t hpet_number;
    uint16_t minimum_tick;
    uint8_t page_protection;
} __attribute__((packed)) hpet_header_t;

void hpet_initialize();
bool configure_timer(int timer, int irq, int ms, bool one_shot);

#endif