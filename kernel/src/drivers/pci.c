#include "pci.h"
#include <sys/dev.h>
#include <lib/panic.h>
#include <drivers/acpi.h>
#include <drivers/ahci.h>
#include <memory/hhdm.h>
#include <memory/heap.h>

#ifdef __ARCH_AMD64
#include <arch/amd64/port.h>

#define PORT_CONFIG_ADDRESS 0xCF8
#define PORT_CONFIG_DATA 0xCFC
#endif

#define VENDOR_INVALID 0xFFFF
#define CMD_REG_BUSMASTER (1 << 2)
#define HEADER_TYPE_MULTIFUNC (1 << 7)

typedef struct {
    uint64_t base_address;
    uint16_t segment_group_number;
    uint8_t start_bus_number;
    uint8_t end_bus_number;
    uint32_t rsv0;
} __attribute__((packed)) pcie_segment_entry_t;

static pcie_segment_entry_t *g_segments;
list_t g_pci_devices = LIST_INIT;

static uint32_t (*g_read)(pci_device_t *device, uint8_t offset, uint8_t size);
static void (*g_write)(pci_device_t *device, uint8_t offset, uint8_t size, uint32_t value);

static uint32_t readd(pci_device_t *device, uint8_t offset) { return g_read(device, offset, 4); }
static uint16_t readw(pci_device_t *device, uint8_t offset) { return (uint16_t) g_read(device, offset, 2); }
static uint16_t readb(pci_device_t *device, uint8_t offset) { return (uint8_t) g_read(device, offset, 1); }
static void writed(pci_device_t *device, uint8_t offset, uint32_t value) { return g_write(device, offset, 4, value); }
static void writew(pci_device_t *device, uint8_t offset, uint16_t value) { return g_write(device, offset, 2, value); }
static void writeb(pci_device_t *device, uint8_t offset, uint8_t value) { return g_write(device, offset, 1, value); }

static uint32_t pcie_read(pci_device_t *device, uint8_t offset, uint8_t size) {
    uint32_t register_offset = offset;
    register_offset |= (uint32_t) (device->bus - g_segments[device->segment].start_bus_number) << 20;
    register_offset |= (uint32_t) (device->slot & 0x1F) << 15;
    register_offset |= (uint32_t) (device->func & 0x7) << 12;
    volatile void *address = (void *) HHDM(g_segments[device->segment].base_address) + register_offset;

    switch(size) {
        case 4: return *(volatile uint32_t *) address;
        case 2: return *(volatile uint16_t *) address;
        case 1: return *(volatile uint8_t *) address;
    }
    __builtin_unreachable();
}

static void pcie_write(pci_device_t *device, uint8_t offset, uint8_t size, uint32_t value) {
    uint32_t register_offset = offset;
    register_offset |= (uint32_t) (device->bus - g_segments[device->segment].start_bus_number) << 20;
    register_offset |= (uint32_t) (device->slot & 0x1F) << 15;
    register_offset |= (uint32_t) (device->func & 0x7) << 12;
    volatile void *address = (void *) (HHDM(g_segments[device->segment].base_address) + register_offset);

    switch(size) {
        case 4: *(volatile uint32_t *) address = (uint32_t) value; return;
        case 2: *(volatile uint16_t *) address = (uint16_t) value; return;
        case 1: *(volatile uint8_t *) address = (uint8_t) value; return;
    }
}

#ifdef __ARCH_AMD64
static uint32_t pci_read(pci_device_t *device, uint8_t offset, uint8_t size) {
    uint32_t register_offset = (offset & 0xFC) | (1 << 31);
    register_offset |= (uint32_t) device->bus << 16;
    register_offset |= (uint32_t) (device->slot & 0x1F) << 11;
    register_offset |= (uint32_t) (device->func & 0x7) << 8;
    port_outl(PORT_CONFIG_ADDRESS, register_offset);

    uint32_t value = port_inl(PORT_CONFIG_DATA);
    switch(size) {
        case 4: return value;
        case 2: return (uint16_t) (value >> ((offset & 2) * 8));
        case 1: return (uint8_t) (value >> ((offset & 3) * 8));
    }
    __builtin_unreachable();
}

static void pci_write(pci_device_t *device, uint8_t offset, uint8_t size, uint32_t value) {
    uint32_t original = pci_read(device, offset, size);

    switch(size) {
        case 4: original = value; break;
        case 2:
            original &= ~(0xFFFF >> ((offset & 2) * 8));
            original |= (uint16_t) value >> ((offset & 2) * 8);
            break;
        case 1:
            original &= ~(0xFF >> ((offset & 3) * 8));
            original |= (uint8_t) value >> ((offset & 3) * 8);
            break;
    }
    port_outl(PORT_CONFIG_ADDRESS, ((uint32_t) device->bus << 16) | ((uint32_t) (device->slot & 0x1F) << 11) | ((uint32_t) (device->func & 0x7) << 8) | (offset & 0xFC) | (1 << 31));
    port_outl(PORT_CONFIG_DATA, original);
}
#endif

static void check_bus(uint16_t segment, uint8_t bus);
static void check_function(uint16_t segment, uint8_t bus, uint8_t slot, uint8_t func) {
    pci_device_t *device = heap_alloc(sizeof(pci_device_t));
    device->segment = segment;
    device->bus = bus;
    device->slot = slot;
    device->func = func;

    uint16_t vendor_id = readw(device, offsetof(pci_device_header_t, vendor_id));
    if(vendor_id == VENDOR_INVALID) {
        heap_free(device);
        return;
    }
    list_insert_behind(&g_pci_devices, &device->list);

    uint8_t class = readb(device, offsetof(pci_device_header_t, class));
    uint8_t sub_class = readb(device, offsetof(pci_device_header_t, sub_class));
    uint8_t prog_if = readb(device, offsetof(pci_device_header_t, program_interface));

    if(class == 0x6 && sub_class == 0x4) {
        check_bus(segment, (uint8_t) (readb(device, offsetof(pci_header1_t, secondary_bus)) >> 8));
        return;
    }

    dev_driver_t *driver;
    DEV_FOREACH(driver) {
        if(driver->type != DEV_DRIVER_PCI) continue;
        if((driver->pci->match & PCI_DRIVER_MATCH_CLASS) && driver->pci->class != class) continue;
        if((driver->pci->match & PCI_DRIVER_MATCH_SUBCLASS) && driver->pci->subclass != sub_class) continue;
        if((driver->pci->match & PCI_DRIVER_MATCH_FUNCTION) && driver->pci->prog_if != prog_if) continue;
        driver->pci->initialize(device);
    }
}

static void check_bus(uint16_t segment, uint8_t bus) {
    for(uint8_t slot = 0; slot < 32; slot++) {
        pci_device_t device = { .segment = segment, .bus = bus, .slot = slot };
        if(readw(&device, offsetof(pci_device_header_t, vendor_id)) == VENDOR_INVALID) continue;
        for(uint8_t func = 0; func < ((readb(&device, offsetof(pci_device_header_t, header_type)) & HEADER_TYPE_MULTIFUNC) ? 8 : 1); func++)
            check_function(segment, bus, slot, func);
    }
}

static void check_segment(uint16_t segment) {
    pci_device_t root_controller = { .segment = segment };
    if(readb(&root_controller, offsetof(pci_device_header_t, header_type)) & HEADER_TYPE_MULTIFUNC) {
        for(uint8_t func = 0; func < 8; func++) {
            pci_device_t controller = { .segment = segment, .func = func };
            if(readw(&controller, offsetof(pci_device_header_t, vendor_id)) == VENDOR_INVALID) break;
            check_bus(segment, func);
        }
    } else {
        check_bus(segment, 0);
    }
}

uint8_t pci_config_read_byte(pci_device_t *device, uint8_t offset) { return readb(device, offset); }
uint16_t pci_config_read_word(pci_device_t *device, uint8_t offset) { return readw(device, offset); }
uint32_t pci_config_read_double(pci_device_t *device, uint8_t offset) { return readd(device, offset); }

void pci_config_write_byte(pci_device_t *device, uint8_t offset, uint8_t data) { writeb(device, offset, data); }
void pci_config_write_word(pci_device_t *device, uint8_t offset, uint16_t data) { writew(device, offset, data); }
void pci_config_write_double(pci_device_t *device, uint8_t offset, uint32_t data) { writed(device, offset, data); }

pci_bar_t *pci_config_read_bar(pci_device_t *device, uint8_t index) {
    if(index > 5) return 0;

    pci_bar_t *new_bar = heap_alloc(sizeof(pci_bar_t));
    uint8_t offset = sizeof(pci_device_header_t) + index * sizeof(uint32_t);

    uint32_t bar = pci_config_read_double(device, offset);
    pci_config_write_double(device, offset, ~0);
    uint32_t size = pci_config_read_double(device, offset);
    pci_config_write_double(device, offset, bar);

    if(bar & 1) {
        new_bar->iospace = true;
        new_bar->address = (bar & ~0b11);
        new_bar->size = ~(size & ~0b11) + 1;
    } else {
        new_bar->iospace = false;
        new_bar->address = (bar & ~0b1111);
        new_bar->size = ~(size & ~0b1111) + 1;

        int type = ((bar >> 1) & 0b11);
        switch(type) {
            case 0: break;
            case 2:
                new_bar->address |= (uint64_t) pci_config_read_double(device, offset + sizeof(uint32_t)) << 32;
                break;
            default:
                heap_free(new_bar);
                return 0;
        }
    }
    return new_bar;
}

void pci_enumerate(acpi_sdt_header_t *mcfg) {
    if(mcfg) {
        g_segments = (pcie_segment_entry_t *) ((uintptr_t) mcfg + sizeof(acpi_sdt_header_t) + 8);
        g_read = &pcie_read;
        g_write = &pcie_write;
        unsigned int entry_count = (mcfg->length - (sizeof(acpi_sdt_header_t) + 8)) / sizeof(pcie_segment_entry_t);
        for(unsigned int i = 0; i < entry_count; i++) check_segment(i);
    } else {
#ifdef __ARCH_AMD64
        g_read = &pci_read;
        g_write = &pci_write;
        check_segment(0);
#else
        panic("PCI: Legacy PCI unavailable");
#endif
    }
}