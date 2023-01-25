#include "gdt.h"

static gdt_entry_t g_gdt[] = {
    {0},
    {
        .limit = 0,
        .base_low = 0,
        .base_mid = 0,
        .access = 0b10011010,
        .granularity = 0b00100000,
        .base_high = 0
    },
    {
        .limit = 0,
        .base_low = 0,
        .base_mid = 0,
        .access = 0b10010010,
        .granularity = 0b00000000,
        .base_high = 0
    }
};

void gdt_initialize() {
    gdt_descriptor_t gdtr;
    gdtr.limit = sizeof(g_gdt) - 1;
    gdtr.base = (uint64_t) &g_gdt;
    asm volatile("lgdt %0" : : "m" (gdtr));
}