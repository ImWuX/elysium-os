#include "vmm.h"
#include <memory/pmm.h>
#include <util/util.h>

page_table_t *pml4;

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

static uint8_t pt_get_flag(uint64_t entry, pt_entry_flags_t flag){
    uint64_t bitSelector = 1 << flag;
    return (entry & bitSelector) > 0;
}

static uint64_t get_index(uint64_t address, int level) {
    address >>= 3 + (3 - level + 1) * 9;
    return address & 0x1FF;
}

static page_table_t *new_table(page_table_t *dest_table, uint64_t index) {
    uint64_t new_address = (uint64_t) request_page();
    page_table_t *new_table = (page_table_t *) (HHDM_BASE + new_address);
    memset(0, (void *) new_table, 0x1000);

    uint64_t new_entry = 0;
    pt_set_address(&new_entry, new_address >> 12);
    pt_set_flag(&new_entry, PT_FLAG_PRESENT, 1);
    pt_set_flag(&new_entry, PT_FLAG_READ_WRITE, 1);
    dest_table->entries[index] = new_entry;
    return new_table;
}

void initialize_memory(uint64_t pml4_address) {
    pml4 = (page_table_t *) (HHDM_BASE + pml4_address);
}

void map_memory(void *physical_address, void *virtual_address) {
    page_table_t *current_table = pml4;
    for(int i = 0; i < 3; i++) {
        uint64_t entry = current_table->entries[get_index((uint64_t) virtual_address, i)];
        if(!pt_get_flag(entry, PT_FLAG_PRESENT)) {
            current_table = new_table(current_table, get_index((uint64_t) virtual_address, i));
        } else {
            current_table = (page_table_t *) (HHDM_BASE + pt_get_address(entry));
        }
    }

    uint64_t entry = 0;
    pt_set_address(&entry, (uint64_t) physical_address >> 12);
    pt_set_flag(&entry, PT_FLAG_PRESENT, 1);
    pt_set_flag(&entry, PT_FLAG_READ_WRITE, 1);
    current_table->entries[get_index((uint64_t) virtual_address, 3)] = entry;
}

uint64_t get_physical_address(uint64_t virtual_address) {
    page_table_t *current_table = pml4;
    for(int i = 0; i < 3; i++) {
        uint64_t entry = current_table->entries[get_index(virtual_address, i)];
        if(!pt_get_flag(entry, PT_FLAG_PRESENT)) {
            return 0;
        } else {
            current_table = (page_table_t *) (HHDM_BASE + pt_get_address(entry));
        }
    }
    uint64_t entry = current_table->entries[get_index(virtual_address, 3)];
    return !pt_get_flag(entry, PT_FLAG_PRESENT) ? 0 : pt_get_address(entry);
}

uint64_t get_hhdm_address(uint64_t physical_address) {
    return HHDM_BASE + physical_address;
}