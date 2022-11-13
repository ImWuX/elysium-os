#include "idt.h"
#include <util/util.h>

#define ENTRIES 256

static idt_entry_t g_idt[ENTRIES];
static idt_descriptor_t g_idt_descriptor = { (uint16_t) sizeof(g_idt) - 1, (uint64_t) &g_idt };

void idt_set_gate(uint8_t gate, uint64_t handler, uint16_t segment, uint8_t flags) {
    g_idt[gate].low_offset = (uint16_t) (handler & 0xFFFF);
    g_idt[gate].segment_selector = segment;
    g_idt[gate].ist = 0;
    g_idt[gate].flags = flags;
    g_idt[gate].middle_offset = (uint16_t) ((handler >> 16) & 0xFFFF);
    g_idt[gate].high_offset = (uint32_t) ((handler >> 32) & 0xFFFFFFFF);
    g_idt[gate].rsv0 = 0;
}

void idt_enable_gate(uint8_t gate) {
    FLAG_SET(g_idt[gate].flags, IDT_FLAG_PRESENT);
}

void idt_disable_gate(uint8_t gate) {
    FLAG_UNSET(g_idt[gate].flags, IDT_FLAG_PRESENT);
}

void initialize_idt() {
    asm volatile("lidt (%0)" : : "r" (&g_idt_descriptor));
}