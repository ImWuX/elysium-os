#include "irq.h"
#include <cpu/pic8259.h>
#include <cpu/apic.h>
#include <cpu/idt.h>

#define SEG 0x8

static interrupt_handler_t g_interrupt_handlers[256];

void irq_initialize() {
    idt_set_gate(32, (uintptr_t) irq_32, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(33, (uintptr_t) irq_33, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(34, (uintptr_t) irq_34, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(35, (uintptr_t) irq_35, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(36, (uintptr_t) irq_36, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(37, (uintptr_t) irq_37, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(38, (uintptr_t) irq_38, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(39, (uintptr_t) irq_39, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(40, (uintptr_t) irq_40, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(41, (uintptr_t) irq_41, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(42, (uintptr_t) irq_42, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(43, (uintptr_t) irq_43, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(44, (uintptr_t) irq_44, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(45, (uintptr_t) irq_45, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(46, (uintptr_t) irq_46, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(47, (uintptr_t) irq_47, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
}

void irq_register_handler(uint8_t id, interrupt_handler_t interrupt_handler) {
    g_interrupt_handlers[id] = interrupt_handler;
}

// TODO: Pass regs as a pointer, HERE AND IN EXCEPTIOn
void irq_handler(irq_frame_t regs) {
    if(g_interrupt_handlers[regs.int_no]) g_interrupt_handlers[regs.int_no](regs);
    apic_eoi(regs.int_no);
    pic8259_eoi(regs.int_no >= 40);
}