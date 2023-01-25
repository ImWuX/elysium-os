#ifndef CPU_GDT_H
#define CPU_GDT_H

#include <stdint.h>

typedef struct {
    uint16_t limit;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t granularity;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdt_descriptor_t;

void gdt_initialize();

#endif