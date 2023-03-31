#include "idt.h"
#include <panic.h>

#define ENTRY_COUNT 256

static idt_entry_t g_idt[ENTRY_COUNT];

void idt_initialize() {
    idt_descriptor_t idtr;
    idtr.limit = sizeof(idt_entry_t) * ENTRY_COUNT - 1;
    idtr.base = (uint64_t) &g_idt;
    asm volatile("lidt %0" : : "m" (idtr));
}

void idt_set_gate(uint8_t gate, uintptr_t handler, uint16_t segment, uint8_t flags) {
    g_idt[gate].low_offset = (uint16_t) handler;
    g_idt[gate].segment_selector = segment;
    g_idt[gate].ist = 0;
    g_idt[gate].flags = flags;
    g_idt[gate].middle_offset = (uint16_t) (handler >> 16);
    g_idt[gate].high_offset = (uint32_t) (handler >> 32);
    g_idt[gate].rsv0 = 0;
}

int idt_free_vector(idt_ipl_t ipl) {
    for(int i = ipl << 4; i < ENTRY_COUNT; i++) {
        if(g_idt[i].flags & IDT_FLAG_PRESENT) continue;
        return i;
    }
    return IDT_ERR_FULL;
}