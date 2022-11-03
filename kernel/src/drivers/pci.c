#include "pci.h"
#include <drivers/ports.h>
#include <drivers/acpi.h>
#include <drivers/ahci.h>
#include <memory/vmm.h>

static uint16_t pci_config_readWord(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t lbus  = (uint32_t) bus;
    uint32_t lslot = (uint32_t) slot;
    uint32_t lfunc = (uint32_t) func;
 
    outl(0xCF8, (uint32_t) ((lbus << 16) | (lslot << 11) | (lfunc << 8) | (offset & 0xFC) | ((uint32_t) 0x80000000)));

    return (uint16_t) ((inl(0xCFC) >> ((offset & 2) * 8)) & 0xFFFF);
}

void pci_enumerate() {
    for(int bus = 0; bus < 256; bus++) {
        for(int slot = 0; slot < 32; slot++) {
            uint16_t res = pci_config_readWord(bus, slot, 0, 0);
            if(res == 0xFFFF) continue;
            int func_count = 1;
            if(((pci_config_readWord(bus, slot, 0, 0xE) & 0xF0) & 0x80) != 0) {
                func_count = 8;
            }
            for(int func = 0; func < func_count; func++) {
                res = pci_config_readWord(bus, slot, func, 0);
                if(res == 0xFFFF) continue;

                uint16_t class_bits = pci_config_readWord(bus, slot, func, 0xA);
                uint8_t class = class_bits >> 8;
                uint8_t sub_class = class_bits & 0xF;
                uint16_t prog_if = pci_config_readWord(bus, slot, func, 0x8) >> 8;
                if(class == 0x1 && sub_class == 0x6 && prog_if == 0x1) {
                    uint32_t bar5 = ((uint32_t) pci_config_readWord(bus, slot, func, 0x26)) << 16;
                    bar5 += pci_config_readWord(bus, slot, func, 0x24);
                    initialize_ahci_device((uint64_t) bar5);
                }
            }
        }
    }
}

static void pci_express_enumerate_function(uint64_t device_address, uint8_t function) {
    uint64_t function_address = device_address + ((uint64_t) function << 12);

    map_memory((void *) function_address, (void *) function_address);
    pci_device_header_t *header = (pci_device_header_t *) function_address;
    if(!header->device_id || header->device_id == 0xFFFF) return;

    switch(header->class) {
        case 0x1:
            switch(header->sub_class) {
                case 0x6:
                    switch(header->program_interface) {
                        case 0x1:
                            pci_header0_t *header0 = (pci_header0_t *) header;
                            initialize_ahci_device(header0->bar5);
                            break;
                    }
                    break;
            }
            break;
    }
}

static void pci_express_enumerate_device(uint64_t bus_address, uint8_t device) {
    uint64_t device_address = bus_address + ((uint64_t) device << 15);

    map_memory((void *) device_address, (void *) device_address);
    pci_device_header_t *header = (pci_device_header_t *) device_address;
    if(!header->device_id || header->device_id == 0xFFFF) return;

    for(uint8_t function = 0; function < 8; function++) {
        pci_express_enumerate_function(device_address, function);
    }
}

static void pci_express_enumerate_bus(uint64_t base_address, uint8_t bus) {
    uint64_t bus_address = base_address + ((uint64_t) bus << 20);

    map_memory((void *) bus_address, (void *) bus_address);
    pci_device_header_t *header = (pci_device_header_t *) bus_address;
    if(!header->device_id || header->device_id == 0xFFFF) return;

    for(uint8_t device = 0; device < 32; device++) {
        pci_express_enumerate_device(bus_address, device);
    }
}

void pci_express_enumerate(sdt_header_t *mcfg) {
    int entry_count = (mcfg->length - (sizeof(sdt_header_t) + 8)) / sizeof(pcie_device_entry_t);
    pcie_device_entry_t *entry = (pcie_device_entry_t *) (((void *) mcfg) + sizeof(sdt_header_t) + 8);
    for(int i = 0; i < entry_count; i++) {
        for(uint8_t bus = entry->start_bus_number; bus < entry->end_bus_number; bus++) {
            pci_express_enumerate_bus(entry->base_address, bus);
        }
        entry++;
    }

}