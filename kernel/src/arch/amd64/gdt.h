#ifndef ARCH_AMD64_GDT_H
#define ARCH_AMD64_GDT_H

#include <stdint.h>

#define GDT_CODE_RING0 0x8
#define GDT_DATA_RING0 0x10
#define GDT_CODE_RING3 0x20
#define GDT_DATA_RING3 0x18

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
 * @brief Loads the GDT
 */
void gdt_load();

// void gdt_tss_initialize();

#endif