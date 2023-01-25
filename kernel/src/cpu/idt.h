#ifndef CPU_IDT_H
#define CPU_IDT_H

#include <stdint.h>

typedef struct {
    uint16_t low_offset;
    uint16_t segment_selector;
    uint8_t ist;
    uint8_t flags;
    uint16_t middle_offset;
    uint32_t high_offset;
    uint32_t rsv0;
} __attribute__((packed)) idt_entry_t;

typedef struct {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed)) idt_descriptor_t;

typedef enum {
    IDT_FLAG_GATE_64BIT_INT     = 0xE,
    IDT_FLAG_GATE_64BIT_TRAP    = 0xF,

    IDT_FLAG_RING0              = (0 << 5),
    IDT_FLAG_RING1              = (1 << 5),
    IDT_FLAG_RING2              = (2 << 5),
    IDT_FLAG_RING3              = (3 << 5),

    IDT_FLAG_PRESENT            = 0x80
} idt_flags_t;

void idt_initialize();
void idt_enable_gate(uint8_t gate);
void idt_disable_gate(uint8_t gate);
void idt_set_gate(uint8_t gate, uint64_t handler, uint16_t segment, uint8_t flags);

#endif