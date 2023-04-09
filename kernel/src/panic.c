#include "panic.h"
#include <stdbool.h>
#include <string.h>
#include <cpu/interrupt.h>
#include <graphics/basicfont.h>

#define DEFAULTX 50
#define DEFAULTY 50

static draw_context_t *g_ctx;
static char *g_symbols;
static uint64_t g_symbols_size;
static int g_x, g_y;

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

static void printc(char c) {
    switch(c) {
        case '\n':
            g_x = DEFAULTX;
            g_y += BASICFONT_HEIGHT;
            break;
        default:
            draw_char(g_ctx, g_x, g_y, c, 0xFFFFFFFF);
            g_x += BASICFONT_WIDTH;
            break;
    }
}

static void print(char *str) {
    while(*str) printc(*str++);
}

static void print_num(uint64_t value) {
    uint64_t pw = 1;
    while(value / pw >= 16) pw *= 16;

    printc('0');
    printc('x');
    while(pw > 0) {
        uint8_t c = value / pw;
        if(c >= 10) {
            printc(c - 10 + 'a');
        } else {
            printc(c + '0');
        }
        value %= pw;
        pw /= 16;
    }
}

static void reset() {
    draw_rect(g_ctx, 0, 0, g_ctx->width, g_ctx->height, draw_color(g_ctx, 255, 60, 60));
    g_x = DEFAULTX;
    g_y = DEFAULTY;
}

typedef struct stack_frame {
    struct stack_frame *rbp;
    uint64_t rip;
} __attribute__((packed)) stack_frame_t;

static void stack_trace(stack_frame_t *stack_frame) {
    print("Stack Trace:\n");
    for(int i = 0; stack_frame && stack_frame->rip && i < 30; i++) {
        uint64_t offset = 0;
        uint64_t address = 0;
        bool skip = false;
        for(uint64_t j = 0; j < g_symbols_size; j++) {
            if(g_symbols[j] == '\n') {
                skip = false;
                address = 0;
                continue;
            }
            if(skip) continue;
            if(g_symbols[j] >= '0' && g_symbols[j] <= '9') {
                address *= 16;
                address += g_symbols[j] - '0';
                continue;
            }
            if(g_symbols[j] >= 'a' && g_symbols[j] <= 'f') {
                address *= 16;
                address += g_symbols[j] - 'a' + 10;
                continue;
            }
            if(g_symbols[j] == ' ' && address >= stack_frame->rip) break;
            skip = true;
            offset = j + 3;
        }

        print("    ");
        if(offset >= g_symbols_size) {
            print("[UNKNOWN]");
        } else {
            while(g_symbols[offset] != '\n') printc(g_symbols[offset++]);
        }
        print(" <");
        print_num((uint64_t) stack_frame->rip);
        print(">\n");
        stack_frame = stack_frame->rbp;
    }
}

void panic_initialize(draw_context_t *ctx, char *symbols, uint64_t symbols_size) {
    g_ctx = ctx;
    g_symbols = symbols;
    g_symbols_size = symbols_size;
}

noreturn void panic(char *location, char *msg) {
    reset();
    print("KERNEL PANIC\n");
    print(location);
    printc('\n');
    print(msg);
    printc('\n');
    stack_frame_t *stack_frame;
    asm volatile("movq %%rbp, %0" : "=r" (stack_frame));
    stack_trace(stack_frame);
    asm volatile("cli\nhlt");
    __builtin_unreachable();
}

noreturn void exception_handler(interrupt_frame_t *regs) {
    reset();
    uint64_t cr2_value;
    asm volatile("movq %%cr2, %0" : "=r" (cr2_value));
    print("EXCEPTION\n");
    print(g_exception_messages[regs->int_no]);
    print("\nr15: "); print_num(regs->r15);
    print("\nr14: "); print_num(regs->r14);
    print("\nr13: "); print_num(regs->r13);
    print("\nr12: "); print_num(regs->r12);
    print("\nr11: "); print_num(regs->r11);
    print("\nr10: "); print_num(regs->r10);
    print("\nr9: "); print_num(regs->r9);
    print("\nr8: "); print_num(regs->r8);
    print("\nrdi: "); print_num(regs->rdi);
    print("\nrsi: "); print_num(regs->rsi);
    print("\nrbp: "); print_num(regs->rbp);
    print("\nrdx: "); print_num(regs->rdx);
    print("\nrcx: "); print_num(regs->rcx);
    print("\nrbx: "); print_num(regs->rbx);
    print("\nrax: "); print_num(regs->rax);
    print("\nint_no: "); print_num(regs->int_no);
    print("\nerr_code: "); print_num(regs->err_code);
    print("\ncr2: "); print_num(cr2_value);
    print("\nrip: "); print_num(regs->rip);
    print("\ncs: "); print_num(regs->cs);
    print("\nrflags: "); print_num(regs->rflags);
    print("\nrsp: "); print_num(regs->rsp);
    print("\nss: "); print_num(regs->ss);
    printc('\n');
    stack_frame_t initial_stack_frame;
    initial_stack_frame.rbp = regs->rbp;
    initial_stack_frame.rip = regs->rip;
    stack_trace(&initial_stack_frame);
    asm volatile("cli\nhlt");
    __builtin_unreachable();
}