#include "exception.h"
#include <common/panic.h>

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

[[noreturn]] void x86_64_exception_unhandled(x86_64_interrupt_frame_t *frame) {
    stack_frame_t initial_stack_frame;
    initial_stack_frame.rbp = (stack_frame_t *) frame->rbp;
    initial_stack_frame.rip = frame->rip;

    uint64_t cr2_value;
    asm volatile("movq %%cr2, %0" : "=r" (cr2_value));
    panic_stack_trace(
        &initial_stack_frame,
        "UNHANDLED EXCEPTION\n%s\nr15: %#lx\nr14: %#lx\nr13: %#lx\nr12: %#lx\nr11: %#lx\nr10: %#lx\nr9: %#lx\nr8: %#lx\n"
        "rdi: %#lx\nrsi: %#lx\nrbp: %#lx\nrdx: %#lx\nrcx: %#lx\nrbx: %#lx\nrax: %#lx\nint_no: %#lx\nerr_code: %#lx\ncr2: %#lx\nrip: %#lx\ncs: %#lx\n"
        "rflags: %#lx\nrsp: %#lx\nss: %#lx",
        g_exception_messages[frame->int_no],
        frame->r15,
        frame->r14,
        frame->r13,
        frame->r12,
        frame->r11,
        frame->r10,
        frame->r9,
        frame->r8,
        frame->rdi,
        frame->rsi,
        frame->rbp,
        frame->rdx,
        frame->rcx,
        frame->rbx,
        frame->rax,
        frame->int_no,
        frame->err_code,
        cr2_value,
        frame->rip,
        frame->cs,
        frame->rflags,
        frame->rsp,
        frame->ss
    );
    __builtin_unreachable();
}