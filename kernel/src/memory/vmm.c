#include "vmm.h"
#include <stdbool.h>
#include <stdio.h>
#include <memory/pmm.h>
#include <memory/hhdm.h>
#include <tmplibc.h>

static vmm_page_table_t *g_pml4;

static void pt_set_address(uint64_t *entry, uint64_t address) {
    address &= 0x000FFFFFFFFFF000;
    *entry &= 0xFFF0000000000FFF;
    *entry |= address;
}

static uint64_t pt_get_address(uint64_t entry) {
    return entry & 0x000FFFFFFFFFF000;
}

static void pt_set_flag(uint64_t *entry, vmm_pt_flags_t flag, bool enabled) {
    if(enabled) {
        BIT_SET(*entry, flag);
    } else {
        BIT_UNSET(*entry, flag);
    }
}

static bool pt_get_flag(uint64_t entry, vmm_pt_flags_t flag) {
    return entry & (1 << flag);
}

static uint64_t address_to_index(uint64_t address, uint8_t level) {
    return (address >> (3 + (4 - level) * 9)) & 0x1FF;
}

static void tlb_flush() {
    uint64_t cache;
    asm volatile("mov %%cr3, %0" : "=r" (cache));
    asm volatile("mov %0, %%cr3" : : "r" (cache));
}

void vmm_initialize(uint64_t pml4_address) {
    g_pml4 = (vmm_page_table_t *) HHDM(pml4_address);

    uint64_t sp;
    asm volatile("mov %%rsp, %0" : "=rm" (sp));
    asm volatile("mov %0, %%rsp" : : "rm" (HHDM(sp)));
    uint64_t bp;
    asm volatile("mov %%rbp, %0" : "=rm" (bp));
    asm volatile("mov %0, %%rbp" : : "rm" (HHDM(bp)));

    for(int i = 0; i < 256; i++) {
        g_pml4->entries[i] = 0;
    }

    tlb_flush();
}

void vmm_mapf(void *physical_address, void *virtual_address, uint64_t flags) {
    vmm_page_table_t *current_table = g_pml4;
    for(uint8_t i = 0; i < 3; i++) {
        uint64_t index = address_to_index((uint64_t) virtual_address, i);
        uint64_t entry = current_table->entries[index];
        if(!pt_get_flag(entry, VMM_PT_FLAG_PRESENT)) {
            uint64_t free_address = (uint64_t) pmm_page_alloc();
            vmm_page_table_t *new_table = (vmm_page_table_t *) HHDM(free_address);
            memset(0, new_table, 0x1000);

            uint64_t new_entry = 0;
            pt_set_address(&new_entry, free_address);
            pt_set_flag(&new_entry, VMM_PT_FLAG_PRESENT, true);
            pt_set_flag(&new_entry, VMM_PT_FLAG_READWRITE, true);
            current_table->entries[index] = new_entry;
            current_table = new_table;

        } else {
            current_table = (vmm_page_table_t *) HHDM(pt_get_address(entry));
        }
    }

    flags &= 0x11FF;

    uint64_t entry = 0;
    pt_set_address(&entry, (uint64_t) physical_address);
    pt_set_flag(&entry, VMM_PT_FLAG_PRESENT, true);
    entry |= flags;
    current_table->entries[address_to_index((uint64_t) virtual_address, 3)] = entry;
}

void vmm_map(void *physical_address, void *virtual_address) {
    vmm_mapf(physical_address, virtual_address, VMM_PT_FLAG_READWRITE);
}