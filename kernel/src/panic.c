#include "panic.h"
#include <stdio.h>

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

[[noreturn]] void panic(char *location, char *msg) {
    printf("KERNEL PANIC\n[%s] %s\n", location, msg);
    asm volatile("cli");
    while(true) asm volatile("hlt");
    __builtin_unreachable();
}

[[noreturn]] void panic_exception(interrupt_frame_t *frame) {
    uint64_t cr2_value;
    asm volatile("movq %%cr2, %0" : "=r" (cr2_value));
    printf("UNHANDLED EXCEPTION\n%s\n", g_exception_messages[frame->int_no]);
    printf("r15: %#lx\n", frame->r15);
    printf("r14: %#lx\n", frame->r14);
    printf("r13: %#lx\n", frame->r13);
    printf("r12: %#lx\n", frame->r12);
    printf("r11: %#lx\n", frame->r11);
    printf("r10: %#lx\n", frame->r10);
    printf("r9: %#lx\n", frame->r9);
    printf("r8: %#lx\n", frame->r8);
    printf("rdi: %#lx\n", frame->rdi);
    printf("rsi: %#lx\n", frame->rsi);
    printf("rbp: %#lx\n", frame->rbp);
    printf("rdx: %#lx\n", frame->rdx);
    printf("rcx: %#lx\n", frame->rcx);
    printf("rbx: %#lx\n", frame->rbx);
    printf("rax: %#lx\n", frame->rax);
    printf("int_no: %#lx\n", frame->int_no);
    printf("err_code: %#lx\n", frame->err_code);
    printf("cr2: %#lx\n", cr2_value);
    printf("rip: %#lx\n", frame->rip);
    printf("cs: %#lx\n", frame->cs);
    printf("rflags: %#lx\n", frame->rflags);
    printf("rsp: %#lx\n", frame->rsp);
    printf("ss: %#lx\n", frame->ss);
    asm volatile("cli");
    while(true) asm volatile("hlt");
    __builtin_unreachable();
}