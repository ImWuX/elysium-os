#include "isr.h"
#include <drivers/ports.h>
#include <cpu/idt.h>
#include <stdio.h>

#define FLAG 0x8E
#define SEG 0x08

static interrupt_handler_t g_interrupt_handlers[256];

char *exception_messages[] = {
    "Division by Zero",
    "Debug",
    "Non-Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Out of Bounds",
    "Invalid Opcode",
    "No Coprocessor",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Bat TSS",
    "Segment not Present",
    "Stack Fault",
    "General Protection Fault",
    "Page Fault",
    "Unknown Interrupt",
    "Coprocessor Fault",
    "Alignment Check",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved"
};

void initialize_isrs() {
    set_idt_gate(0, (uint64_t) isr_0, SEG, FLAG);
    set_idt_gate(1, (uint64_t) isr_1, SEG, FLAG);
    set_idt_gate(2, (uint64_t) isr_2, SEG, FLAG);
    set_idt_gate(3, (uint64_t) isr_3, SEG, FLAG);
    set_idt_gate(4, (uint64_t) isr_4, SEG, FLAG);
    set_idt_gate(5, (uint64_t) isr_5, SEG, FLAG);
    set_idt_gate(6, (uint64_t) isr_6, SEG, FLAG);
    set_idt_gate(7, (uint64_t) isr_7, SEG, FLAG);
    set_idt_gate(8, (uint64_t) isr_8, SEG, FLAG);
    set_idt_gate(9, (uint64_t) isr_9, SEG, FLAG);
    set_idt_gate(10, (uint64_t) isr_10, SEG, FLAG);
    set_idt_gate(11, (uint64_t) isr_11, SEG, FLAG);
    set_idt_gate(12, (uint64_t) isr_12, SEG, FLAG);
    set_idt_gate(13, (uint64_t) isr_13, SEG, FLAG);
    set_idt_gate(14, (uint64_t) isr_14, SEG, FLAG);
    set_idt_gate(15, (uint64_t) isr_15, SEG, FLAG);
    set_idt_gate(16, (uint64_t) isr_16, SEG, FLAG);
    set_idt_gate(17, (uint64_t) isr_17, SEG, FLAG);
    set_idt_gate(18, (uint64_t) isr_18, SEG, FLAG);
    set_idt_gate(19, (uint64_t) isr_19, SEG, FLAG);
    set_idt_gate(20, (uint64_t) isr_20, SEG, FLAG);
    set_idt_gate(21, (uint64_t) isr_21, SEG, FLAG);
    set_idt_gate(22, (uint64_t) isr_22, SEG, FLAG);
    set_idt_gate(23, (uint64_t) isr_23, SEG, FLAG);
    set_idt_gate(24, (uint64_t) isr_24, SEG, FLAG);
    set_idt_gate(25, (uint64_t) isr_25, SEG, FLAG);
    set_idt_gate(26, (uint64_t) isr_26, SEG, FLAG);
    set_idt_gate(27, (uint64_t) isr_27, SEG, FLAG);
    set_idt_gate(28, (uint64_t) isr_28, SEG, FLAG);
    set_idt_gate(29, (uint64_t) isr_29, SEG, FLAG);
    set_idt_gate(30, (uint64_t) isr_30, SEG, FLAG);
    set_idt_gate(31, (uint64_t) isr_31, SEG, FLAG);
}

void initialize_irqs() {
    // Remap the PIC
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0x0);
    outb(0xA1, 0x0); 

    // Install the IRQs
    set_idt_gate(IRQ_OFFSET, (uint64_t) irq_0, SEG, FLAG);
    set_idt_gate(IRQ_OFFSET + 1, (uint64_t) irq_1, SEG, FLAG);
    set_idt_gate(IRQ_OFFSET + 2, (uint64_t) irq_2, SEG, FLAG);
    set_idt_gate(IRQ_OFFSET + 3, (uint64_t) irq_3, SEG, FLAG);
    set_idt_gate(IRQ_OFFSET + 4, (uint64_t) irq_4, SEG, FLAG);
    set_idt_gate(IRQ_OFFSET + 5, (uint64_t) irq_5, SEG, FLAG);
    set_idt_gate(IRQ_OFFSET + 6, (uint64_t) irq_6, SEG, FLAG);
    set_idt_gate(IRQ_OFFSET + 7, (uint64_t) irq_7, SEG, FLAG);
    set_idt_gate(IRQ_OFFSET + 8, (uint64_t) irq_8, SEG, FLAG);
    set_idt_gate(IRQ_OFFSET + 9, (uint64_t) irq_9, SEG, FLAG);
    set_idt_gate(IRQ_OFFSET + 10, (uint64_t) irq_10, SEG, FLAG);
    set_idt_gate(IRQ_OFFSET + 11, (uint64_t) irq_11, SEG, FLAG);
    set_idt_gate(IRQ_OFFSET + 12, (uint64_t) irq_12, SEG, FLAG);
    set_idt_gate(IRQ_OFFSET + 13, (uint64_t) irq_13, SEG, FLAG);
    set_idt_gate(IRQ_OFFSET + 14, (uint64_t) irq_14, SEG, FLAG);
    set_idt_gate(IRQ_OFFSET + 15, (uint64_t) irq_15, SEG, FLAG);
}

void register_interrupt_handler(uint8_t id, interrupt_handler_t interrupt_handler) {
    g_interrupt_handlers[id] = interrupt_handler;
}

void isr_handler(cpu_register_t regs) {
    char *message = exception_messages[regs.int_no];
    printf("Exception: %s %x %x\n", message, regs.err_code, regs.rip);
    asm ("hlt");
}

void irq_handler(cpu_register_t regs) {
    if(regs.int_no >= 40) outb(0xA0, 0x20); // Reset IRQS On Slave
    outb(0x20, 0x20); // Reset IRQS On Master
    if(g_interrupt_handlers[regs.int_no] != 0) g_interrupt_handlers[regs.int_no](regs);
}