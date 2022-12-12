#include "vmm.h"
#include <pmm.h>
#include <util.h>

static page_table_t *g_pml4;

static void pt_set_address(uint64_t *entry, uint64_t address) {
    address &= 0x000000ffffffffff;
    *entry &= 0xfff0000000000fff;
    *entry |= (address << 12);
}

static uint64_t pt_get_address(uint64_t entry) {
    return entry & 0x000ffffffffff000;
}

static void pt_set_flag(uint64_t *entry, pt_entry_flags_t flag, uint8_t enabled) {
    uint64_t bitSelector = 1 << flag;
    *entry &= ~bitSelector;
    if(enabled) *entry |= bitSelector;
}

static uint8_t pt_get_flag(uint64_t entry, pt_entry_flags_t flag) {
    uint64_t bitSelector = 1 << flag;
    return (entry & bitSelector) > 0;
}

void vmm_initialize(boot_memap_entry_t *memory_map, uint64_t memory_map_length) {
    g_pml4 = (page_table_t *) pmm_request_page();
    memset(0, (uint8_t *) g_pml4, 0x1000);

    for(uint64_t i = 0; i < memory_map_length; i++) {
        if(memory_map[i].type != BOOT_MEMAP_TYPE_USABLE) continue;
        uint64_t address = memory_map[i].base_address;
        address &= 0xFFFFFFFFFFFFF000;
        while(address < memory_map[i].base_address + memory_map[i].length) {
            if(address % 0x200000 == 0 && address + 0x200000 < memory_map[i].base_address + memory_map[i].length) {
                vmm_map_memory_2mb((void *) address, (void *) 0xFFFF800000000000 + address);
                address += 0x200000;
            } else {
                vmm_map_memory((void *) address, (void *) 0xFFFF800000000000 + address);
                address += 0x1000;
            }
        }
    }

    // TODO: Need to start using HHDM
    for(uint64_t i = 0; i < 512; i++) {
        vmm_map_memory((void *) (i * 0x1000), (void *) (i * 0x1000));
    }
    asm volatile("mov %0, %%cr3" : : "r" (g_pml4));

}

void vmm_map_memory(void *physical_address, void *virtual_address) {
    uint64_t indexes[4];
    uint64_t address = (uint64_t) virtual_address;
    address >>= 3;
    for(int i = 0; i <= 3; i++) {
        address >>= 9;
        indexes[3 - i] = address & 0x1ff;
    }

    page_table_t *current_table = g_pml4;
    for(int i = 0; i <= 2; i++) {
        uint64_t entry = current_table->entries[indexes[i]];
        if(!pt_get_flag(entry, PT_FLAG_PRESENT)) {
            page_table_t *new_table = (page_table_t *) pmm_request_page();
            memset(0, (uint8_t *) new_table, 0x1000);
            uint64_t new_entry = 0;
            pt_set_address(&new_entry, (uint64_t) new_table >> 12);
            pt_set_flag(&new_entry, PT_FLAG_PRESENT, 1);
            pt_set_flag(&new_entry, PT_FLAG_READ_WRITE, 1);
            current_table->entries[indexes[i]] = new_entry;
            current_table = new_table;
        } else {
            current_table = (page_table_t *) pt_get_address(entry);
        }
    }

    uint64_t entry = 0;
    pt_set_address(&entry, (uint64_t) physical_address >> 12);
    pt_set_flag(&entry, PT_FLAG_PRESENT, 1);
    pt_set_flag(&entry, PT_FLAG_READ_WRITE, 1);
    current_table->entries[indexes[3]] = entry;
}

void vmm_map_memory_2mb(void *physical_address, void *virtual_address) {
    uint64_t indexes[3];
    uint64_t address = (uint64_t) virtual_address;
    address >>= 12;
    for(int i = 0; i <= 2; i++) {
        address >>= 9;
        indexes[2 - i] = address & 0x1ff;
    }

    page_table_t *current_table = g_pml4;
    for(int i = 0; i <= 1; i++) {
        uint64_t entry = current_table->entries[indexes[i]];
        if(!pt_get_flag(entry, PT_FLAG_PRESENT)) {
            page_table_t *new_table = (page_table_t *) pmm_request_page();
            memset(0, (uint8_t *) new_table, 0x1000);
            uint64_t new_entry = 0;
            pt_set_address(&new_entry, (uint64_t) new_table >> 12);
            pt_set_flag(&new_entry, PT_FLAG_PRESENT, 1);
            pt_set_flag(&new_entry, PT_FLAG_READ_WRITE, 1);
            current_table->entries[indexes[i]] = new_entry;
            current_table = new_table;
        } else {
            current_table = (page_table_t *) pt_get_address(entry);
        }
    }

    uint64_t entry = ((uint64_t) physical_address) & 0xFFFFFFFE00000;
    pt_set_flag(&entry, PT_FLAG_PRESENT, 1);
    pt_set_flag(&entry, PT_FLAG_READ_WRITE, 1);
    pt_set_flag(&entry, 7, 1);
    current_table->entries[indexes[2]] = entry;
}