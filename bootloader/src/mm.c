#include "mm.h"
#include <stdbool.h>
#include <bootlog.h>
#include <util.h>

#define PAGE_SIZE 0x1000
#define BOOTSECTOR_BASE 0x7000
#define MAX_MEMAP_ENTRIES 255
#define LOWEST_MEMORY_BOUNDARY 0x500

extern uint8_t ld_mmap[], ld_bootloader_end[];

boot_memap_entry_t *g_memap;
uint64_t g_memap_length;

static uint64_t align_page_up(uint64_t unaligned_address) {
    return unaligned_address + PAGE_SIZE & ~(uint64_t) (PAGE_SIZE - 1);
}

static void memap_err() {
    boot_log("Memory map exceeded the entry limit\n", LOG_LEVEL_ERROR);
    asm("cli");
    asm("hlt");
}

static bool memap_claim(uint64_t base, uint64_t length, boot_memap_entry_type_t type) {
    for(uint64_t i = 0; i < g_memap_length; i++) {
        if(g_memap[i].base_address + g_memap[i].length <= base) continue;
        if(g_memap[i].type != BOOT_MEMAP_TYPE_USABLE) return false;
        if(g_memap[i].base_address > base) return false;
        uint64_t original_base = g_memap[i].base_address;
        uint64_t original_length = g_memap[i].length;
        boot_memap_entry_type_t original_type = g_memap[i].type;
        if(g_memap[i].base_address != base) {
            g_memap_length++;
            for(uint64_t j = g_memap_length - 1; j > i; j--) g_memap[j] = g_memap[j - 1];
            i++;
            g_memap[i - 1].length = base - original_base;
        }
        g_memap[i].base_address = base;
        g_memap[i].length = length;
        g_memap[i].type = type;
        if(original_base + original_length != base + length) {
            g_memap_length++;
            for(uint64_t j = g_memap_length - 1; j > i; j--) g_memap[j] = g_memap[j - 1];
            g_memap[i + 1].base_address = base + length;
            g_memap[i + 1].length = (original_base + original_length) - (base + length);
            g_memap[i + 1].type = original_type;
        }
        if(i > 0 && g_memap[i - 1].type == type && g_memap[i - 1].base_address + g_memap[i - 1].length == base) {
            g_memap[i - 1].length += length;
            g_memap_length--;
            for(uint64_t j = i; j < g_memap_length; j++) g_memap[j] = g_memap[j + 1];
            i--;
        }
        if(i < g_memap_length - 1 && g_memap[i + 1].type == type && g_memap[i + 1].base_address == base + length) {
            g_memap[i].length += g_memap[i + 1].length;
            g_memap_length--;
            for(uint64_t j = i + 1; j < g_memap_length; j++) g_memap[j] = g_memap[j + 1];
        }

        if(g_memap_length > MAX_MEMAP_ENTRIES) memap_err();
        return true;
    }
    return false;
}

void mm_initialize() {
    uint16_t e820_length = *(uint16_t *) ld_mmap;
    e820_entry_t *e820 = (e820_entry_t *) (ld_mmap + 2);

    g_memap = ld_bootloader_end;
    g_memap_length = 0;
    // TODO: Map should be ensured to be ordered, sections of same type to be contigous, and not overlapping?
    for(uint16_t i = 0; i < e820_length; i++) {
        boot_memap_entry_t entry;
        entry.base_address = e820[i].address;
        entry.length = e820[i].length;
        switch(e820[i].type) {
            case E820_TYPE_USABLE:
                entry.type = BOOT_MEMAP_TYPE_USABLE;
                break;
            case E820_TYPE_BAD:
                entry.type = BOOT_MEMAP_TYPE_BAD;
                break;
            case E820_TYPE_ACPI_RECLAIMABLE:
                entry.type = BOOT_MEMAP_TYPE_ACPI_RECLAIMABLE;
                break;
            case E820_TYPE_ACPI_NVS:
                entry.type = BOOT_MEMAP_TYPE_ACPI_NVS;
                break;
            default:
                entry.type = BOOT_MEMAP_TYPE_RESERVED;
                break;
        }

        if(entry.base_address < LOWEST_MEMORY_BOUNDARY) {
            entry.length -= LOWEST_MEMORY_BOUNDARY - entry.base_address;
            entry.base_address = LOWEST_MEMORY_BOUNDARY;
        }

        if(g_memap_length > 0) {
            boot_memap_entry_t *last_entry = &g_memap[g_memap_length - 1];
            uint64_t last_end = last_entry->base_address + last_entry->length;
            if(last_end > entry.base_address) {
                if(last_entry->type > entry.type) {
                    entry.length -= last_end - entry.base_address;
                    entry.base_address = last_end;
                } else {
                    last_entry->length = entry.base_address - last_entry->base_address;
                }
                if(last_entry->type == entry.type) {
                    last_entry->length = (entry.base_address + entry.length) - last_entry->base_address;
                    continue;
                }
            }
            if(last_end == entry.base_address && last_entry->type == entry.type) {
                last_entry->length = (entry.base_address + entry.length) - last_entry->base_address;
                continue;
            }
        }

        if((entry.type == BOOT_MEMAP_TYPE_USABLE || entry.type == BOOT_MEMAP_TYPE_BOOT_RECLAIMABLE) && entry.base_address % PAGE_SIZE != 0) {
            uint64_t aligned_address = align_page_up(entry.base_address);
            entry.length -= aligned_address - entry.base_address;
            entry.base_address = aligned_address;
            entry.length = entry.length & ~0xFFF;
            if(entry.length == 0) continue;
        }

        g_memap[g_memap_length] = entry;
        g_memap_length++;

        if(g_memap_length > MAX_MEMAP_ENTRIES) memap_err();
    }
    memap_claim(BOOTSECTOR_BASE, align_page_up((uint64_t) (&g_memap + MAX_MEMAP_ENTRIES * sizeof(boot_memap_entry_t)) - BOOTSECTOR_BASE), BOOT_MEMAP_TYPE_BOOT_RECLAIMABLE);
}

void *mm_request_linear_pages_type(uint64_t number_of_pages, boot_memap_entry_type_t type) {
    uint64_t size = number_of_pages * PAGE_SIZE;
    for(uint64_t i = 0; i < g_memap_length; i++) {
        boot_memap_entry_t entry = g_memap[i];
        if(entry.type != BOOT_MEMAP_TYPE_USABLE) continue;
        if(entry.length < size) continue;
        if(!memap_claim(entry.base_address, size, type)) continue;
        return entry.base_address;
    }
    boot_log("OUT OF MEMORY", LOG_LEVEL_ERROR);
    asm("cli");
    asm("hlt");
}

void *mm_request_linear_pages(uint64_t number_of_pages) {
    mm_request_linear_pages_type(number_of_pages, BOOT_MEMAP_TYPE_BOOT_RECLAIMABLE);
}

void *mm_request_page() {
    return mm_request_linear_pages(1);
}