#include "interrupt.h"
#include <lib/kprint.h>
#include <arch/interrupt.h>
#include <arch/x86_64/gdt.h>

#define FLAGS_NORMAL 0x8E
#define FLAGS_TRAP 0x8F
#define IDT_SIZE 256

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

typedef struct {
    bool free;
    interrupt_handler_t handler;
    interrupt_priority_t priority;
} interrupt_entry_t;

static interrupt_priority_t g_ipl_to_interrupt_map[] = {
    [ARCH_INTERRUPT_IPL_NORMAL] = INTERRUPT_PRIORITY_NORMAL,
    [ARCH_INTERRUPT_IPL_IPC] = INTERRUPT_PRIORITY_IPC,
    [ARCH_INTERRUPT_IPL_CRITICAL] = INTERRUPT_PRIORITY_CRITICAL
};

static arch_interrupt_ipl_t g_interrupt_to_ipl_map[] = {
    [INTERRUPT_PRIORITY_NORMAL] = ARCH_INTERRUPT_IPL_NORMAL,
    [INTERRUPT_PRIORITY_IPC] = ARCH_INTERRUPT_IPL_IPC,
    [INTERRUPT_PRIORITY_CRITICAL] = ARCH_INTERRUPT_IPL_CRITICAL
};

extern uint64_t g_isr_stubs[IDT_SIZE];

static idt_entry_t g_idt[IDT_SIZE];
static interrupt_entry_t g_entries[IDT_SIZE];

interrupt_irq_eoi_t g_interrupt_irq_eoi;

static void set_idt_gate(uint8_t gate, uintptr_t handler, uint16_t segment, uint8_t flags) {
    g_idt[gate].low_offset = (uint16_t) handler;
    g_idt[gate].middle_offset = (uint16_t) (handler >> 16);
    g_idt[gate].high_offset = (uint32_t) (handler >> 32);
    g_idt[gate].segment_selector = segment;
    g_idt[gate].flags = flags;
    g_idt[gate].ist = 0;
    g_idt[gate].rsv0 = 0;
}

static void ipl_set(interrupt_priority_t priority) {
    asm volatile("mov %0, %%cr8" : : "r" ((uint64_t) priority));
}

static interrupt_priority_t ipl_get() {
    uint64_t ipl;
    asm volatile("mov %%cr8, %0" : "=r" (ipl));
    return (interrupt_priority_t) ipl;
}

void interrupt_handler(interrupt_frame_t *frame) {
    if(frame->int_no >= 0x20) g_interrupt_irq_eoi(frame->int_no);
    if(!g_entries[frame->int_no].free) g_entries[frame->int_no].handler(frame);
}

void interrupt_init() {
    for(unsigned long i = 0; i < sizeof(g_idt) / sizeof(idt_entry_t); i++) {
        set_idt_gate(i, g_isr_stubs[i], GDT_CODE_RING0, FLAGS_NORMAL);
        g_entries[i].free = true;
    }
}

void interrupt_load_idt() {
    idt_descriptor_t idtr;
    idtr.limit = sizeof(g_idt) - 1;
    idtr.base = (uint64_t) &g_idt;
    asm volatile("lidt %0" : : "m" (idtr));
}

void interrupt_set(uint8_t vector, interrupt_priority_t priority, interrupt_handler_t handler) {
    g_entries[vector].free = false;
    g_entries[vector].handler = handler;
    g_entries[vector].priority = priority;
}

int interrupt_request(interrupt_priority_t priority, interrupt_handler_t handler) {
    for(int i = priority << 4; i < IDT_SIZE; i++) {
        if(!g_entries[i].free) continue;
        interrupt_set(i, priority, handler);
        return i;
    }
    return -1;
}

arch_interrupt_ipl_t arch_interrupt_ipl(arch_interrupt_ipl_t ipl) {
    arch_interrupt_ipl_t old = g_interrupt_to_ipl_map[ipl_get()];
    ipl_set(g_ipl_to_interrupt_map[ipl]);
    return old;
}