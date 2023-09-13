#include "gdt.h"

#define RSP_SEGMENT 0x28
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

static gdt_entry_t g_gdt[] = {
    {},
    {
        .limit = 0,
        .base_low = 0,
        .base_mid = 0,
        .access = 0b10011010,
        .flags = 0b00100000,
        .base_high = 0
    },
    {
        .limit = 0,
        .base_low = 0,
        .base_mid = 0,
        .access = 0b10010010,
        .flags = 0b00000000,
        .base_high = 0
    },
    {
        .limit = 0,
        .base_low = 0,
        .base_mid = 0,
        .access = 0b11110010,
        .flags = 0b00000000,
        .base_high = 0
    },
    {
        .limit = 0,
        .base_low = 0,
        .base_mid = 0,
        .access = 0b11111010,
        .flags = 0b00100000,
        .base_high = 0
    },
    {}, {}
};

void gdt_load() {
    gdt_descriptor_t gdtr;
    gdtr.limit = sizeof(g_gdt) - 1;
    gdtr.base = (uint64_t) &g_gdt;
    asm volatile("lgdt %0\npush $0x8\nlea 1f(%%rip), %%rax\npush %%rax\nlretq\n1:\nmov $0x10, %%rax\nmov %%rax, %%ds\nmov %%rax, %%ss\nmov %%rax, %%es\nmov %%rax, %%fs\nmov %%rax, %%gs\n" : : "m" (gdtr) : "rax", "memory");
}

void gdt_load_tss(tss_t *tss) {
    gdt_system_entry_t *entry = (gdt_system_entry_t *) ((uintptr_t) g_gdt + RSP_SEGMENT);
    entry->entry.access = 0b10001001;
    entry->entry.flags = (1 << 4) | (((uint8_t) (sizeof(tss_t) << 16)) & 0b00001111);
    entry->entry.limit = (uint16_t) sizeof(tss_t);
    entry->entry.base_low = (uint16_t) (uint64_t) tss;
    entry->entry.base_mid = (uint8_t) ((uint64_t) tss >> 16);
    entry->entry.base_high = (uint8_t) ((uint64_t) tss >> 24);
    entry->base_ext = (uint32_t) ((uint64_t) tss >> 32);

    uint16_t segment = RSP_SEGMENT;
    asm volatile("ltr %0" : : "m" (segment));
}