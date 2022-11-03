#ifndef DRIVERS_PCI_H
#define DRIVERS_PCI_H

#include <stdint.h>

typedef struct {
    uint64_t base_address;
    uint16_t segment_group_number;
    uint8_t start_bus_number;
    uint8_t end_bus_number;
    uint32_t rsv0;
} __attribute__((packed)) pcie_device_entry_t;

typedef struct {
    uint16_t vendor_id;
    uint16_t device_id;
    uint16_t command;
    uint16_t status;
    uint8_t revision_id;
    uint8_t program_interface;
    uint8_t sub_class;
    uint8_t class;
    uint8_t cache_line_size;
    uint8_t latency_timer;
    uint8_t header_type;
    uint8_t bist;
} __attribute__((packed)) pci_device_header_t;

typedef struct {
    pci_device_header_t device_header;

    uint32_t bar0;
    uint32_t bar1;
    uint32_t bar2;
    uint32_t bar3;
    uint32_t bar4;
    uint32_t bar5;

    uint32_t cardbus_cis_pointer;
    uint16_t subsystem_vendor_id;
    uint16_t subsystem_id;
    uint32_t expansion_rom_base_address;
    uint8_t capabilities_pointer;
    uint8_t rsv0;
    uint16_t rsv1;
    uint32_t rsv2;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint8_t min_grant;
    uint8_t max_latency;
} __attribute__((packed)) pci_header0_t;

void pci_enumerate();
void pci_express_enumerate();

#endif