#include "gdt.h"

#define STR(S) #S
#define XSTR(S) STR(S)

typedef struct {
    uint16_t limit;
    uint16_t base_low;
    uint8_t base_mid;
    uint8_t access;
    uint8_t flags;
    uint8_t base_high;
} __attribute__((packed)) gdt_entry_t;

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
    }
};

void x86_64_gdt_load() {
    gdt_descriptor_t gdtr;
    gdtr.limit = sizeof(g_gdt) - 1;
    gdtr.base = (uint64_t) &g_gdt;
    asm volatile(
        "lgdt %0\n"
        "push " XSTR(X86_64_GDT_CODE_RING0) "\n"
        "lea 1f(%%rip), %%rax\n"
        "push %%rax\n"
        "lretq\n"
        "1:\n"
        "mov " XSTR(X86_64_GDT_DATA_RING0) ", %%rax\n"
        "mov %%rax, %%ds\n"
        "mov %%rax, %%ss\n"
        "mov %%rax, %%es\n"
        "mov %%rax, %%fs\n"
        "mov %%rax, %%gs\n"
        : : "m" (gdtr) : "rax", "memory"
    );
}