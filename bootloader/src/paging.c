#include "paging.h"
#include <memory.h>
#include <bootlog.h>

uint64_t free_memory;
uint64_t reserved_memory;
uint64_t used_memory;

uint64_t buffer_size;
uint8_t *buffer;

void initialize_paging() {
    uint16_t entry_count = *(uint16_t *) &ld_mmap;
    e820_entry_t *entry = (e820_entry_t *) ((uint8_t *) &ld_mmap + 2);

    uint64_t total_memory_size = 0;
    uint64_t largest_entry_size = 0;
    uint64_t largest_entry_address = 0;
    for(uint32_t i = 0; i < entry_count; i++) {
        if(entry[i].length > largest_entry_size && entry[i].type == E820_TYPE_USABLE) {
            largest_entry_size = entry[i].length;
            largest_entry_address = entry[i].address;
        }
        total_memory_size += entry[i].length;
    }
    free_memory = total_memory_size;
    reserved_memory = 0;
    used_memory = 0;

    buffer = (uint8_t *) largest_entry_address;
    buffer_size = total_memory_size / 0x1000 / 8 + 1;
    for(uint64_t i = 0; i < buffer_size; i++) {
        buffer[i] = 0;
    }

    // Reserve sections provided by bios
    for(uint32_t i = 0; i < entry_count; i++) {
        if(entry[i].type == 1) continue;
        reserve_pages((void *) entry[i].address, entry[i].length / 0x1000 + 1);
    }

    // Reserve the page buffer
    reserve_pages(buffer, buffer_size / 0x1000 + 1);

    // TODO: Reserve everything under 0x100000 for now.
    reserve_pages(0, 0x100);
}

void *request_page() {
    for(uint64_t i = 0; i < buffer_size; i++) {
        for(uint8_t j = 0; j < 8; j++) {
            uint8_t mask = 0b10000000 >> j;
            if((buffer[i] & mask) == 0) {
                void *address = (void *) ((i * 8 + j) * 0x1000);
                lock_page(address);
                return address;
            }
        }
    }
    boot_log("(Critical) Failed to allocate a page. Out of memory.", LOG_LEVEL_ERROR);
    return 0;
}

void *request_linear_pages(int count) {
    int available_linearly = 0;
    for(uint64_t i = 0; i < buffer_size; i++) {
        for(uint8_t j = 0; j < 8; j++) {
            uint8_t mask = 0b10000000 >> j;
            if((buffer[i] & mask) == 0) {
                available_linearly++;
                if(available_linearly >= count) {
                    uint64_t address = (i * 8 + j) * 0x1000;
                    while(count > 0) {
                        lock_page((void *) address);
                        count--;
                        address -= 0x1000;
                    }
                    return (void *) (address + 0x1000);
                }
                continue;
            }
            available_linearly = 0;
        }
    }
    boot_log("(Critical) Failed to allocate a page. Out of memory.", LOG_LEVEL_ERROR);
    return 0;
}

uint8_t page_state(void *address) {
    uint64_t index = (uint64_t) address / 0x1000 / 8;
    if(index > buffer_size) return 2;
    uint8_t bit_index = (uint64_t) address / 0x1000 % 8;
    uint8_t mask = 0b10000000 >> bit_index;
    return (buffer[index] & mask) > 0;
}

void reserve_page(void *address) {
    uint64_t index = (uint64_t) address / 0x1000 / 8;
    if(index > buffer_size) return;
    uint8_t bit_index = (uint64_t) address / 0x1000 % 8;
    uint8_t mask = 0b10000000 >> bit_index;
    buffer[index] &= ~mask;
    buffer[index] |= mask;
    free_memory -= 0x1000;
    reserved_memory += 0x1000;
}

void reserve_pages(void *address, uint64_t count) {
    for(uint64_t i = 0; i < count; i++) {
        reserve_page((void *)((uint64_t *) address + i * 0x1000 / 8));
    }
}

void lock_page(void *address) {
    uint64_t index = (uint64_t) address / 0x1000 / 8;
    if(index > buffer_size) return;
    uint8_t bit_index = (uint64_t) address / 0x1000 % 8;
    uint8_t mask = 0b10000000 >> bit_index;
    buffer[index] &= ~mask;
    buffer[index] |= mask;
    free_memory -= 0x1000;
    used_memory += 0x1000;
}

void lock_pages(void *address, uint64_t count) {
    for(uint64_t i = 0; i < count; i++) {
        lock_page((void *)((uint64_t *) address + i * 0x1000 / 8));
    }
}

void free_page(void *address) {
    uint64_t index = (uint64_t) address / 0x1000 / 8;
    if(index > buffer_size) return;
    uint8_t bit_index = (uint64_t) address / 0x1000 % 8;
    uint8_t mask = 0b10000000 >> bit_index;
    buffer[index] &= ~mask;
    free_memory += 0x1000;
    used_memory -= 0x1000;
}

void free_pages(void *address, uint64_t count) {
    for(uint64_t i = 0; i < count; i++) {
        free_page((void *)((uint64_t *) address + i * 0x1000 / 8));
    }
}

uint64_t get_free_memory() {
    return free_memory;
}

uint64_t get_used_memory() {
    return used_memory;
}

uint64_t get_reserved_memory() {
    return reserved_memory;
}

uint64_t get_buffer_address() {
    return (uint64_t) buffer;
}

uint64_t get_buffer_size() {
    return buffer_size;
}