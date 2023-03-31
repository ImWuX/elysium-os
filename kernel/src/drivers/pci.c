#include "pci.h"
#include <drivers/ports.h>
#include <drivers/acpi.h>
#include <drivers/ahci.h>
#include <memory/hhdm.h>
#include <memory/heap.h>
#include <memory/vmm.h>

#define PORT_CONFIG_ADDRESS 0xCF8
#define PORT_CONFIG_DATA 0xCFC

#define VENDOR_INVALID 0xFFFF
#define CMD_REG_BUSMASTER (1 << 2)
#define HEADER_TYPE_MULTIFUNC (1 << 7)

static pcie_segment_entry_t *g_segments;
pci_device_t *g_pci_devices;

static uint32_t readd(uint16_t segment, uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    if(g_segments) bus -= g_segments[segment].start_bus_number;
    uint32_t reg = ((uint32_t) bus << 16) | ((uint32_t) (device & 0x1F) << 11) | ((uint32_t) (func & 0x7) << 8);
    offset &= 0xFC;
    if(g_segments) {
        return *((uint32_t *) (HHDM(g_segments[segment].base_address) + ((reg << 4) | offset)));
    } else {
        ports_outl(PORT_CONFIG_ADDRESS, reg | offset | (1 << 31));
        return ports_inl(PORT_CONFIG_DATA);
    }
}

static uint16_t readw(uint16_t segment, uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    return (uint16_t) (readd(segment, bus, device, func, offset) >> ((offset & 2) * 8));
}

static uint8_t readb(uint16_t segment, uint8_t bus, uint8_t device, uint8_t func, uint8_t offset) {
    return (uint8_t) (readd(segment, bus, device, func, offset) >> ((offset & 3) * 8));
}

static void writed(uint16_t segment, uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint32_t value) {
    if(g_segments) bus -= g_segments[segment].start_bus_number;
    uint32_t reg = ((uint32_t) bus << 16) | ((uint32_t) (device & 0x1F) << 11) | ((uint32_t) (func & 0x7) << 8);
    offset &= 0xFC;
    if(g_segments) {
        *((uint32_t *) (HHDM(g_segments[segment].base_address) + ((reg << 4) | offset))) = value;
    } else {
        ports_outl(PORT_CONFIG_ADDRESS, reg | offset | (1 << 31));
        ports_outl(PORT_CONFIG_DATA, value);
    }
}

static void writew(uint16_t segment, uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint16_t value) {
    uint32_t ovalue = readd(segment, bus, device, func, offset);
    ovalue |= 0xFFFF >> ~((offset & 2) * 8);
    ovalue |= value >> ((offset & 2) * 8);
    writed(segment, bus, device, func, offset, ovalue);
}

static void writeb(uint16_t segment, uint8_t bus, uint8_t device, uint8_t func, uint8_t offset, uint8_t value) {
    uint32_t ovalue = readd(segment, bus, device, func, offset);
    ovalue |= 0xFF >> ~((offset & 3) * 8);
    ovalue |= value >> ((offset & 3) * 8);
    writed(segment, bus, device, func, offset, ovalue);
}

static void check_bus(uint16_t segment, uint8_t bus);
static void check_function(uint16_t segment, uint8_t bus, uint8_t device, uint8_t func) {
    uint16_t vendor_id = readw(segment, bus, device, func, __builtin_offsetof(pci_device_header_t, vendor_id));
    if(vendor_id == VENDOR_INVALID) return;

    pci_device_t *pci_device = heap_alloc(sizeof(pci_device_t));
    pci_device->segment = segment;
    pci_device->bus = bus;
    pci_device->slot = device;
    pci_device->func = func;
    pci_device->list = g_pci_devices;
    g_pci_devices = pci_device;

    uint8_t class = readb(segment, bus, device, func, __builtin_offsetof(pci_device_header_t, class));
    uint8_t sub_class = readb(segment, bus, device, func, __builtin_offsetof(pci_device_header_t, sub_class));
    uint8_t prog_if = readb(segment, bus, device, func, __builtin_offsetof(pci_device_header_t, program_interface));
    switch(class) {
        case 0x1:
            switch(sub_class) {
                case 0x6:
                    switch(prog_if) {
                        case 0x1:
                            ahci_initialize_device(pci_device);
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

static void check_bus(uint16_t segment, uint8_t bus) {
    for(uint8_t device = 0; device < 32; device++) {
        if(readw(segment, bus, device, 0, __builtin_offsetof(pci_device_header_t, vendor_id)) == VENDOR_INVALID) continue;
        for(uint8_t func = 0; func < ((readb(segment, bus, device, 0, __builtin_offsetof(pci_device_header_t, header_type)) & HEADER_TYPE_MULTIFUNC) ? 8 : 1); func++) check_function(segment, bus, device, func);
    }
}

static void check_segment(uint16_t segment) {
    if(readb(segment, 0, 0, 0, __builtin_offsetof(pci_device_header_t, header_type)) & HEADER_TYPE_MULTIFUNC) {
        for(uint8_t func = 0; func < 8; func++) {
            if(readw(segment, 0, 0, func, __builtin_offsetof(pci_device_header_t, vendor_id)) == VENDOR_INVALID) break;
            check_bus(segment, func);
        }
    } else {
        check_bus(segment, 0);
    }
}

uint8_t pci_config_read_byte(pci_device_t *device, uint8_t offset) {
    return readb(device->segment, device->bus, device->slot, device->func, offset);
}

uint16_t pci_config_read_word(pci_device_t *device, uint8_t offset) {
    return readw(device->segment, device->bus, device->slot, device->func, offset);
}

uint32_t pci_config_read_double(pci_device_t *device, uint8_t offset) {
    return readd(device->segment, device->bus, device->slot, device->func, offset);
}

void pci_config_write_byte(pci_device_t *device, uint8_t offset, uint8_t data) {
    return writeb(device->segment, device->bus, device->slot, device->func, offset, data);
}

void pci_config_write_word(pci_device_t *device, uint8_t offset, uint16_t data) {
    return writew(device->segment, device->bus, device->slot, device->func, offset, data);
}

void pci_config_write_double(pci_device_t *device, uint8_t offset, uint32_t data) {
    return writed(device->segment, device->bus, device->slot, device->func, offset, data);
}

void pci_enumerate(acpi_sdt_header_t *mcfg) {
    g_segments = (pcie_segment_entry_t *) ((uintptr_t) mcfg + sizeof(acpi_sdt_header_t) + 8);
    unsigned int entry_count = 1;
    if(g_segments) entry_count = (mcfg->length - (sizeof(acpi_sdt_header_t) + 8)) / sizeof(pcie_segment_entry_t);
    for(unsigned int i = 0; i < entry_count; i++) check_segment(i);
}