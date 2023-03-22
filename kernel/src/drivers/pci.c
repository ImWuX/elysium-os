#include "pci.h"
#include <drivers/ports.h>
#include <drivers/acpi.h>
#include <drivers/ahci.h>
#include <memory/hhdm.h>
#include <memory/vmm.h>

#define PORT_CONFIG_ADDRESS 0xCF8
#define PORT_CONFIG_DATA 0xCFC

#define VENDOR_INVALID 0xFFFF
#define CMD_REG_BUSMASTER (1 << 2)
#define HEADER_TYPE_MULTIFUNC (1 << 7)
#define BAR_4BIT_MASK 0xFFFFFFFFFFFFFFF0
#define BAR_2BIT_MASK 0xFFFFFFFFFFFFFFFC

static uint32_t readd(pcie_segment_entry_t *segment, uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    if(segment) bus -= segment->start_bus_number;
    uint32_t reg = ((uint32_t) bus << 16) | ((uint32_t) (device & 0x1F) << 11) | ((uint32_t) (func & 0x7) << 8);
    offset &= 0xFC;
    if(segment) {
        return *((uint32_t *) (HHDM(segment->base_address) + ((reg << 4) | offset)));
    } else {
        ports_outl(PORT_CONFIG_ADDRESS, reg | offset | (1 << 31));
        return ports_inl(PORT_CONFIG_DATA);
    }
}

static uint16_t readw(pcie_segment_entry_t *segment, uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    return (uint16_t) (readd(segment, bus, device, func, offset) >> ((offset & 2) * 8));
}

static uint8_t readb(pcie_segment_entry_t *segment, uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    return (uint8_t) (readd(segment, bus, device, func, offset) >> ((offset & 3) * 8));
}

static void writed(pcie_segment_entry_t *segment, uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value) {
    if(segment) bus -= segment->start_bus_number;
    uint32_t reg = ((uint32_t) bus << 16) | ((uint32_t) (device & 0x1F) << 11) | ((uint32_t) (func & 0x7) << 8);
    offset &= 0xFC;
    if(segment) {
        *((uint32_t *) (HHDM(segment->base_address) + ((reg << 4) | offset))) = value;
    } else {
        ports_outl(PORT_CONFIG_ADDRESS, reg | offset | (1 << 31));
        ports_outl(PORT_CONFIG_DATA, value);
    }
}

static void check_bus(pcie_segment_entry_t *segment, uint8_t bus);
static void check_function(pcie_segment_entry_t *segment, uint8_t bus, uint8_t device, uint8_t func) {
    uint16_t vendor_id = readw(segment, bus, device, func, __builtin_offsetof(pci_device_header_t, vendor_id));
    if(vendor_id == VENDOR_INVALID) return;

    uint8_t class = readb(segment, bus, device, func, __builtin_offsetof(pci_device_header_t, class));
    uint8_t sub_class = readb(segment, bus, device, func, __builtin_offsetof(pci_device_header_t, sub_class));
    uint16_t prog_if = readb(segment, bus, device, func, __builtin_offsetof(pci_device_header_t, program_interface));
    switch(class) {
        case 0x1:
            switch(sub_class) {
                case 0x6:
                    switch(prog_if) {
                        case 0x1:
                            uint32_t bar5 = readd(segment, bus, device, func, __builtin_offsetof(pci_header0_t, bar5));
                            writed(segment, bus, device, func, __builtin_offsetof(pci_device_header_t, command), readd(segment, bus, device, func, __builtin_offsetof(pci_device_header_t, command)) | (1 << 2));
                            ahci_initialize_device((uintptr_t) bar5 & BAR_4BIT_MASK);
                            break;
                    }
                    break;
            }
            break;
        case 0x6:
            switch(sub_class) {
                case 0x4:
                    check_bus(segment, (uint8_t) (readb(segment, bus, device, func, __builtin_offsetof(pci_header1_t, secondary_bus)) >> 8));
                    break;
            }
            break;
    }
}

static void check_bus(pcie_segment_entry_t *segment, uint8_t bus) {
    for(uint8_t device = 0; device < 32; device++) {
        if(readw(segment, bus, device, 0, __builtin_offsetof(pci_device_header_t, vendor_id)) == VENDOR_INVALID) continue;
        for(uint8_t func = 0; func < ((readb(segment, bus, device, 0, __builtin_offsetof(pci_device_header_t, header_type)) & HEADER_TYPE_MULTIFUNC) ? 8 : 1); func++) check_function(segment, bus, device, func);
    }
}

static void check_segment(pcie_segment_entry_t *segment) {
    if(readb(segment, 0, 0, 0, __builtin_offsetof(pci_device_header_t, header_type)) & HEADER_TYPE_MULTIFUNC) {
        for(uint8_t func = 0; func < 8; func++) {
            if(readw(segment, 0, 0, func, __builtin_offsetof(pci_device_header_t, vendor_id)) == VENDOR_INVALID) break;
            check_bus(segment, func);
        }
    } else {
        check_bus(segment, 0);
    }
}

void pci_enumerate(acpi_sdt_header_t *mcfg) {
    if(mcfg) {
        int entry_count = (mcfg->length - (sizeof(acpi_sdt_header_t) + 8)) / sizeof(pcie_segment_entry_t);
        pcie_segment_entry_t *entry = (pcie_segment_entry_t *) (((void *) mcfg) + sizeof(acpi_sdt_header_t) + 8);
        for(int i = 0; i < entry_count; i++) check_segment(entry++);
    } else {
        check_segment(0);
    }
}