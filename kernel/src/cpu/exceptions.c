#include "exceptions.h"
#include <cpu/idt.h>
#include <stdio.h>
#include <panic.h>

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

void exceptions_handler(exception_cpu_register_t regs) {
    printf("\n---==[ Exception ]==---\n%s\nError Code: %x\nRIP: %x\n---==[    END    ]==---", g_messages[regs.int_no], regs.err_code, regs.rip);
    panic("Exception", g_messages[regs.int_no]);
}

void exceptions_initialize() {
    idt_set_gate(0, (uintptr_t) exception_0, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(1, (uintptr_t) exception_1, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(2, (uintptr_t) exception_2, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(3, (uintptr_t) exception_3, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(4, (uintptr_t) exception_4, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(5, (uintptr_t) exception_5, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(6, (uintptr_t) exception_6, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(7, (uintptr_t) exception_7, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(8, (uintptr_t) exception_8, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(9, (uintptr_t) exception_9, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(10, (uintptr_t) exception_10, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(11, (uintptr_t) exception_11, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(12, (uintptr_t) exception_12, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(13, (uintptr_t) exception_13, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(14, (uintptr_t) exception_14, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(15, (uintptr_t) exception_15, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(16, (uintptr_t) exception_16, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(17, (uintptr_t) exception_17, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(18, (uintptr_t) exception_18, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(19, (uintptr_t) exception_19, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(20, (uintptr_t) exception_20, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(21, (uintptr_t) exception_21, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(22, (uintptr_t) exception_22, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(23, (uintptr_t) exception_23, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(24, (uintptr_t) exception_24, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(25, (uintptr_t) exception_25, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(26, (uintptr_t) exception_26, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(27, (uintptr_t) exception_27, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(28, (uintptr_t) exception_28, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(29, (uintptr_t) exception_29, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(30, (uintptr_t) exception_30, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
    idt_set_gate(31, (uintptr_t) exception_31, SEG, IDT_FLAG_PRESENT | IDT_FLAG_GATE_64BIT_TRAP | IDT_FLAG_RING0);
}