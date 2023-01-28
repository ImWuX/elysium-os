#include "irq.h"
#include <cpu/pic8259.h>
#include <cpu/apic.h>
#include <cpu/idt.h>

#define SEG 0x8

static interrupt_handler_t g_interrupt_handlers[256];

void irq_initialize() {
    idt_set_gate(32, (uint64_t) irq_32, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(33, (uint64_t) irq_33, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(34, (uint64_t) irq_34, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(35, (uint64_t) irq_35, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(36, (uint64_t) irq_36, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(37, (uint64_t) irq_37, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(38, (uint64_t) irq_38, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(39, (uint64_t) irq_39, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(40, (uint64_t) irq_40, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(41, (uint64_t) irq_41, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(42, (uint64_t) irq_42, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(43, (uint64_t) irq_43, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(44, (uint64_t) irq_44, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(45, (uint64_t) irq_45, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(46, (uint64_t) irq_46, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
    idt_set_gate(47, (uint64_t) irq_47, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_INT | IDT_FLAG_RING0);
}

void irq_register_handler(uint8_t id, interrupt_handler_t interrupt_handler) {
    g_interrupt_handlers[id] = interrupt_handler;
}

void irq_handler(irq_cpu_register_t regs) {
    if(g_interrupt_handlers[regs.int_no]) g_interrupt_handlers[regs.int_no](regs);
    apic_eoi(regs.int_no);
    pic8259_eoi(regs.int_no >= 40);
}