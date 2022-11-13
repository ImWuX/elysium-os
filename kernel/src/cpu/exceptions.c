#include "exceptions.h"
#include <cpu/idt.h>
#include <stdio.h>

#define FLAG 0x8E
#define SEG 0x08

static char *g_messages[] = {
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

void exception_handler(exception_cpu_register_t regs) {
    char *message = g_messages[regs.int_no];
    printf("Exception: %s %x %x\n", message, regs.err_code, regs.rip);
    asm ("hlt");
}

void initialize_exceptions() {
    idt_set_gate(0, (uint64_t) exception_0, SEG, FLAG);
    idt_set_gate(1, (uint64_t) exception_1, SEG, FLAG);
    idt_set_gate(2, (uint64_t) exception_2, SEG, FLAG);
    idt_set_gate(3, (uint64_t) exception_3, SEG, FLAG);
    idt_set_gate(4, (uint64_t) exception_4, SEG, FLAG);
    idt_set_gate(5, (uint64_t) exception_5, SEG, FLAG);
    idt_set_gate(6, (uint64_t) exception_6, SEG, FLAG);
    idt_set_gate(7, (uint64_t) exception_7, SEG, FLAG);
    idt_set_gate(8, (uint64_t) exception_8, SEG, FLAG);
    idt_set_gate(9, (uint64_t) exception_9, SEG, FLAG);
    idt_set_gate(10, (uint64_t) exception_10, SEG, FLAG);
    idt_set_gate(11, (uint64_t) exception_11, SEG, FLAG);
    idt_set_gate(12, (uint64_t) exception_12, SEG, FLAG);
    idt_set_gate(13, (uint64_t) exception_13, SEG, FLAG);
    idt_set_gate(14, (uint64_t) exception_14, SEG, FLAG);
    idt_set_gate(15, (uint64_t) exception_15, SEG, FLAG);
    idt_set_gate(16, (uint64_t) exception_16, SEG, FLAG);
    idt_set_gate(17, (uint64_t) exception_17, SEG, FLAG);
    idt_set_gate(18, (uint64_t) exception_18, SEG, FLAG);
    idt_set_gate(19, (uint64_t) exception_19, SEG, FLAG);
    idt_set_gate(20, (uint64_t) exception_20, SEG, FLAG);
    idt_set_gate(21, (uint64_t) exception_21, SEG, FLAG);
    idt_set_gate(22, (uint64_t) exception_22, SEG, FLAG);
    idt_set_gate(23, (uint64_t) exception_23, SEG, FLAG);
    idt_set_gate(24, (uint64_t) exception_24, SEG, FLAG);
    idt_set_gate(25, (uint64_t) exception_25, SEG, FLAG);
    idt_set_gate(26, (uint64_t) exception_26, SEG, FLAG);
    idt_set_gate(27, (uint64_t) exception_27, SEG, FLAG);
    idt_set_gate(28, (uint64_t) exception_28, SEG, FLAG);
    idt_set_gate(29, (uint64_t) exception_29, SEG, FLAG);
    idt_set_gate(30, (uint64_t) exception_30, SEG, FLAG);
    idt_set_gate(31, (uint64_t) exception_31, SEG, FLAG);
}