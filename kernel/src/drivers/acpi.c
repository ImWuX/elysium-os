#include "acpi.h"
#include <lib/mem.h>
#include <common/assert.h>
#include <memory/hhdm.h>

typedef struct {
    uint8_t signature[8];
    uint8_t checksum;
    uint8_t oem[6];
    uint8_t revision;
    uint32_t rsdt_address;
} __attribute__((packed)) acpi_rsdp_t;

typedef struct {
    acpi_rsdp_t rsdp;
    uint32_t length;
    uint64_t xsdt_address;
    uint8_t extended_checksum;
    uint8_t reserved[3];
} __attribute__((packed)) acpi_rsdp_ext_t;

static acpi_sdt_header_t *g_xsdt, *g_rsdt;
static uint8_t g_revision;

static uint8_t checksum(uint8_t *src, uint32_t size) {
    uint32_t checksum = 0;
    for(uint8_t i = 0; i < size; i++) checksum += src[i];
    return (checksum & 1) == 0;
}

void acpi_initialize(void *rsdp) {
    g_revision = ((acpi_rsdp_t *) rsdp)->revision;
    if(g_revision > 0) {
        acpi_rsdp_ext_t *rsdp_ext = (acpi_rsdp_ext_t *) rsdp;
        ASSERT(checksum(((uint8_t *) rsdp_ext) + sizeof(acpi_rsdp_t), sizeof(acpi_rsdp_ext_t) - sizeof(acpi_rsdp_t)));
        g_xsdt = (acpi_sdt_header_t *) HHDM(rsdp_ext->xsdt_address);
    } else {
        g_rsdt = (acpi_sdt_header_t *) HHDM(((acpi_rsdp_t *) rsdp)->rsdt_address);
    }
}

acpi_sdt_header_t *acpi_find_table(uint8_t *signature) {
    if(g_xsdt) {
        uint32_t entry_count = (g_xsdt->length - sizeof(acpi_sdt_header_t)) / sizeof(uint64_t);
        uint64_t *entry_ptr = (uint64_t *) ((uintptr_t) g_xsdt + sizeof(acpi_sdt_header_t));
        for(uint32_t i = 0; i < entry_count; i++) {
            acpi_sdt_header_t *entry = (acpi_sdt_header_t *) HHDM(*entry_ptr);
            if(!memcmp(entry->signature, signature, 4)) return entry;
            entry_ptr++;
        }
    } else if(g_rsdt) {
        uint32_t entry_count = (g_rsdt->length - sizeof(acpi_sdt_header_t)) / sizeof(uint32_t);
        uint32_t *entry_ptr = (uint32_t *) ((uintptr_t) g_rsdt + sizeof(acpi_sdt_header_t));
        for(uint32_t i = 0; i < entry_count; i++) {
            acpi_sdt_header_t *entry = (acpi_sdt_header_t *) HHDM(*entry_ptr);
            if(!memcmp(entry->signature, signature, 4)) return entry;
            entry_ptr++;
        }
    }
    return 0;
}

uint8_t acpi_revision() {
    return g_revision;
}