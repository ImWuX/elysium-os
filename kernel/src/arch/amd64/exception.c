#include "exception.h"
#include <lib/kprint.h>
#include <arch/amd64/lapic.h>

typedef struct stack_frame {
    struct stack_frame *rbp;
    uint64_t rip;
} __attribute__((packed)) stack_frame_t;

static char *g_exception_messages[] = {
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

[[noreturn]] void exception_unhandled(interrupt_frame_t *frame) {
    uint64_t cr2_value;
    asm volatile("movq %%cr2, %0" : "=r" (cr2_value));
    kprintf("UNHANDLED EXCEPTION (CPU %i)\n%s\n", lapic_id(), g_exception_messages[frame->int_no]);
    kprintf("r15: %#lx\n", frame->r15);
    kprintf("r14: %#lx\n", frame->r14);
    kprintf("r13: %#lx\n", frame->r13);
    kprintf("r12: %#lx\n", frame->r12);
    kprintf("r11: %#lx\n", frame->r11);
    kprintf("r10: %#lx\n", frame->r10);
    kprintf("r9: %#lx\n", frame->r9);
    kprintf("r8: %#lx\n", frame->r8);
    kprintf("rdi: %#lx\n", frame->rdi);
    kprintf("rsi: %#lx\n", frame->rsi);
    kprintf("rbp: %#lx\n", frame->rbp);
    kprintf("rdx: %#lx\n", frame->rdx);
    kprintf("rcx: %#lx\n", frame->rcx);
    kprintf("rbx: %#lx\n", frame->rbx);
    kprintf("rax: %#lx\n", frame->rax);
    kprintf("int_no: %#lx\n", frame->int_no);
    kprintf("err_code: %#lx\n", frame->err_code);
    kprintf("cr2: %#lx\n", cr2_value);
    kprintf("rip: %#lx\n", frame->rip);
    kprintf("cs: %#lx\n", frame->cs);
    kprintf("rflags: %#lx\n", frame->rflags);
    kprintf("rsp: %#lx\n", frame->rsp);
    kprintf("ss: %#lx\n", frame->ss);
    asm volatile("cli");
    while(true) asm volatile("hlt");
    __builtin_unreachable();

}