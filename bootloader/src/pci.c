#include "pci.h"
#include <ahci.h>

static uint32_t inl(uint16_t port) {
    uint32_t result;
    asm volatile("inl %1, %0" : "=a" (result) : "Nd" (port));
    return result;
}

static void outl(uint16_t port, uint32_t value) {
    asm volatile("outl %0, %1" : : "a" (value), "Nd" (port));
}

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