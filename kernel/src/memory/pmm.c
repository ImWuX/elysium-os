#include "pmm.h"
#include <memory/vmm.h>

uint64_t free_memory;
uint64_t reserved_memory;
uint64_t used_memory;

uint64_t buffer_size;
uint8_t *buffer;

void initialize_paging(uint64_t buf_address, uint64_t buf_size, uint64_t free, uint64_t reserved, uint64_t used) {
    // TODO: This whole place is a fucking disaster
    buffer_size = buf_size;
    buffer = (uint8_t *) buf_address;
    free_memory = free;
    reserved_memory = reserved;
    used_memory = used;
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
    // TODO: Kernel Panic.. We are out of mem
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
    // TODO: Kernel Panic.. We are out of mem
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