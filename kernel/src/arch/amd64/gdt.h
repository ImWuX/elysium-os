#ifndef ARCH_AMD64_GDT_H
#define ARCH_AMD64_GDT_H

#include <stdint.h>

typedef struct {
    uint16_t limit;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t flags;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
	gdt_entry_t entry;
	uint32_t base_ext;
	uint8_t rsv0;
	uint8_t zero_rsv1;
	uint16_t rsv2;
} __attribute__((packed)) gdt_system_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) gdt_descriptor_t;

typedef struct {
	uint32_t rsv0;
	uint32_t rsp0_lower;
	uint32_t rsp0_upper;
	uint32_t rsp1_lower;
	uint32_t rsp1_upper;
	uint32_t rsp2_lower;
	uint32_t rsp2_upper;
	uint32_t rsv1;
	uint32_t rsv2;
	uint32_t ist1_lower;
	uint32_t ist1_upper;
	uint32_t ist2_lower;
	uint32_t ist2_upper;
	uint32_t ist3_lower;
	uint32_t ist3_upper;
	uint32_t ist4_lower;
	uint32_t ist4_upper;
	uint32_t ist5_lower;
	uint32_t ist5_upper;
	uint32_t ist6_lower;
	uint32_t ist6_upper;
	uint32_t ist7_lower;
	uint32_t ist7_upper;
	uint32_t rsv3;
	uint32_t rsv4;
	uint16_t rsv5;
	uint16_t iomap_base;
} __attribute__((packed)) gdt_tss_t;

/**
 * @brief Initialize GDT
 */
void gdt_initialize();
// void gdt_tss_initialize();

#endif