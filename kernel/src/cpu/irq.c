#include "irq.h"
#include <drivers/ports.h>
#include <cpu/idt.h>
#include <cpu/apic.h>

#define FLAG 0x8E
#define SEG 0x08

static interrupt_handler_t g_interrupt_handlers[256];

void initialize_irqs() {
    idt_set_gate(32, (uint64_t) irq_32, SEG, FLAG);
    idt_set_gate(33, (uint64_t) irq_33, SEG, FLAG);
    idt_set_gate(34, (uint64_t) irq_34, SEG, FLAG);
    idt_set_gate(35, (uint64_t) irq_35, SEG, FLAG);
    idt_set_gate(36, (uint64_t) irq_36, SEG, FLAG);
    idt_set_gate(37, (uint64_t) irq_37, SEG, FLAG);
    idt_set_gate(38, (uint64_t) irq_38, SEG, FLAG);
    idt_set_gate(39, (uint64_t) irq_39, SEG, FLAG);
    idt_set_gate(40, (uint64_t) irq_40, SEG, FLAG);
    idt_set_gate(41, (uint64_t) irq_41, SEG, FLAG);
    idt_set_gate(42, (uint64_t) irq_42, SEG, FLAG);
    idt_set_gate(43, (uint64_t) irq_43, SEG, FLAG);
    idt_set_gate(44, (uint64_t) irq_44, SEG, FLAG);
    idt_set_gate(45, (uint64_t) irq_45, SEG, FLAG);
    idt_set_gate(46, (uint64_t) irq_46, SEG, FLAG);
    idt_set_gate(47, (uint64_t) irq_47, SEG, FLAG);
}

void register_interrupt_handler(uint8_t id, interrupt_handler_t interrupt_handler) {
    g_interrupt_handlers[id] = interrupt_handler;
}

void irq_handler(irq_cpu_register_t regs) {
    if(g_interrupt_handlers[regs.int_no] != 0) g_interrupt_handlers[regs.int_no](regs);
    apic_eoi(regs.int_no);
    if(regs.int_no >= 40) outb(0xA0, 0x20); // Reset IRQS On Slave
    outb(0x20, 0x20); // Reset IRQS On Master
}