#include "acpi.h"
#include <stdbool.h>
#include <stdio.h>
#include <util/util.h>
#include <memory/vmm.h>

#define RSDP_REGION_ONE_BASE 0x80000
#define RSDP_REGION_ONE_END 0x9FFFF
#define RSDP_REGION_TWO_BASE 0xE0000
#define RSDP_REGION_TWO_END 0xFFFFF
#define RSDP_IDENTIFIER "RSD PTR "
#define RSDP_IDENTIFIER_LENGTH 8

static sdt_header_t *g_sdt_ptr;

static void *scan_region(uint8_t *ptr, uint8_t *end) {
    while(ptr <= end - 8) {
        uint8_t match = true;
        for(uint8_t i = 0; i < RSDP_IDENTIFIER_LENGTH; i++) {
            if(ptr[i] != RSDP_IDENTIFIER[i]) {
                match = false;
                break;
            }
        }
        if(match) return (void *) ptr;
        ptr++;
    }
    return 0;
}

static uint8_t checksum(uint8_t *src, uint32_t size) {
    uint32_t checksum = 0;
    for(uint8_t i = 0; i < size; i++) {
        checksum += src[i];
    }
    return (checksum & 1) == 0;
}

static sdt_header_t *load_rsdp() {
    void *ptr = scan_region((uint8_t *) RSDP_REGION_ONE_BASE, (uint8_t *) RSDP_REGION_ONE_END);
    if(ptr == 0) ptr = scan_region((uint8_t *) RSDP_REGION_TWO_BASE, (uint8_t *) RSDP_REGION_TWO_END);
    if(ptr == 0) {
        printe("Failed to locate the ACPI RSDP");
        return 0;
    }

    rsdp_descriptor_t *rsdp_descriptor = (rsdp_descriptor_t *) ptr;
    if(!checksum((uint8_t *) rsdp_descriptor, sizeof(rsdp_descriptor_t))) {
        printe("Checksum failed for the RSDP Descriptor.");
        return 0;
    }

    if(rsdp_descriptor->revision > 0) {
        rsdp_descriptor_ext_t *rsdp_descriptor_ext = (rsdp_descriptor_ext_t *) ptr;
        if(!checksum(((uint8_t *) rsdp_descriptor_ext) + sizeof(rsdp_descriptor_t), sizeof(rsdp_descriptor_ext_t) - sizeof(rsdp_descriptor_t))) {
            printe("Checksum failed for the Extended RSDP Descriptor.");
            return 0;
        }
        return (sdt_header_t *) rsdp_descriptor_ext->xsdt_address;
    } else {
        return (sdt_header_t *) (uint64_t) rsdp_descriptor->rsdt_address;
    }
}

void initialize_acpi() {
    g_sdt_ptr = load_rsdp();
    if(g_sdt_ptr == 0) return printe("ACPI could not be initialized.");
    map_memory((void *) g_sdt_ptr, (void *) g_sdt_ptr);
}

sdt_header_t *acpi_find_table(uint8_t *signature) {
    uint32_t entries = (g_sdt_ptr->length - sizeof(sdt_header_t)) / 4; 
    uint32_t *entry_ptr = (uint32_t *) ((uint64_t) g_sdt_ptr + sizeof(sdt_header_t));
    for(uint32_t i = 0; i < entries; i++) {
        map_memory((void *) (uint64_t) *entry_ptr, (void *) (uint64_t) *entry_ptr);
        sdt_header_t *entry = (sdt_header_t *) (uint64_t) *entry_ptr;
        if(memcmp(entry->signature, signature, 4)) return entry;
        entry_ptr++;
    }
    return 0;
}